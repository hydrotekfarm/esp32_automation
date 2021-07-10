#include "rtc.h"
#include <string.h>
#include <esp_log.h>
#include <esp_sntp.h>
#include <freertos/FreeRTOS.h>

#include "ec_control.h"
#include "ph_control.h"
#include "reservoir_control.h"
#include "rf_transmitter.h"
#include "task_priorities.h"
#include "nvs_namespace_keys.h"
#include "ports.h"
#include "sensor_control.h"

void reservoir_change() {
	set_reservoir_change_flag(true);
	// TODO turn water pump off
	irrigation_timer.active = false;
}

// Enable day time routine
void day() {
	is_day = true;

	lights_on();
	ESP_LOGI("GROW LIGHTS", "Turning lights on");
}

// Enable night time routine
void night() {
	is_day = false;

	lights_off();
	ESP_LOGI("GROW LIGHTS", "Turning lights off");
}

void get_day_night_times(struct tm *day_time, struct tm *night_time) {
	// Get current date time
	struct tm current_date_time;
	ds3231_get_time(&dev, &current_date_time);

	// Set alarm times
	day_time->tm_year = current_date_time.tm_year;
	day_time->tm_mon = current_date_time.tm_mon;
	day_time->tm_mday = current_date_time.tm_mday;
	day_time->tm_hour = day_time_hour;
	day_time->tm_min  = day_time_min;
	day_time->tm_sec = 0;

	night_time->tm_year = current_date_time.tm_year;
	night_time->tm_mon = current_date_time.tm_mon;
	night_time->tm_mday = current_date_time.tm_mday;
	night_time->tm_hour = night_time_hour;
	night_time->tm_min  = night_time_min;
	night_time->tm_sec = 0;
}

void do_nothing() {}

void init_rtc() { // Init RTC
	memset(&dev, 0, sizeof(i2c_dev_t));
	ESP_ERROR_CHECK(ds3231_init_desc(&dev, 0, SDA_GPIO, SCL_GPIO));
	set_time();

	// Lights
	night_time_hour = 21;
	night_time_min = 0;
	day_time_hour  = 6;
	day_time_min = 0;


	// Initialize timers
	init_timer(&irrigation_timer, &irrigation_control, false, false);
	init_timer(control_get_dose_timer(get_ph_control()), &ph_pump_off, false, true);
	init_timer(control_get_wait_timer(get_ph_control()), &do_nothing, false, false);
	init_timer(control_get_dose_timer(get_ec_control()), &ec_dose, false, true);
	init_timer(control_get_wait_timer(get_ec_control()), &do_nothing, false, false);
	init_timer(&reservoir_change_timer, &reservoir_change, false, false);

	// Initialize alarms
	init_alarm(&night_time_alarm, &night, true, false);
	init_alarm(&day_time_alarm, &day, true, false);
}

void init_sntp() {
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
	sntp_init();

	while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET) {
        ESP_LOGI("", "Waiting for system time to be set...");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void set_time() { // Set current time to some date
	time_t now;
	struct tm dateTime;
	time(&now);
	localtime_r(&now, &dateTime);
	ESP_LOGI("", "Current time: %li", now);
	ESP_ERROR_CHECK(ds3231_set_time(&dev, &dateTime));

}

void get_date_time(struct tm *time) {
	// Get current time and set it to return var
	ds3231_get_time(&dev, time);
}

void manage_timers_alarms(void *parameter) {
	const char *TAG = "TIMER_TASK";

	for(;;) {
		// Get current unix time
		time_t unix_time;
		get_unix_time(&dev, &unix_time);

		// Check if timers are done
		check_timer(&dev, &irrigation_timer, unix_time);
		check_timer(&dev, control_get_dose_timer(get_ph_control()), unix_time);
		check_timer(&dev, control_get_wait_timer(get_ph_control()), unix_time);
		check_timer(&dev, control_get_dose_timer(get_ec_control()), unix_time);
		check_timer(&dev, control_get_wait_timer(get_ec_control()), unix_time);

		// Check if alarms are done
		check_alarm(&dev, &night_time_alarm, unix_time);
		check_alarm(&dev, &day_time_alarm, unix_time);
		check_alarm(&dev, get_reservoir_alarm(), unix_time);

		// Check if any timer or alarm is urgent
		bool urgent = (irrigation_timer.active && irrigation_timer.high_priority) || (get_ph_control()->dose_timer.active && get_ph_control()->dose_timer.high_priority) || (get_ph_control()->wait_timer.active && get_ph_control()->wait_timer.high_priority) || (get_ec_control()->dose_timer.active && get_ec_control()->dose_timer.high_priority) || (get_ec_control()->wait_timer.active && get_ec_control()->wait_timer.high_priority) || (night_time_alarm.alarm_timer.active && night_time_alarm.alarm_timer.high_priority) || (day_time_alarm.alarm_timer.active && day_time_alarm.alarm_timer.high_priority);

		// Set priority and delay based on urgency of timers and alarms
		vTaskPrioritySet(timer_alarm_task_handle, urgent ? (configMAX_PRIORITIES - 1) : TIMER_ALARM_TASK_PRIORITY);
		vTaskDelay(pdMS_TO_TICKS(urgent ? TIMER_ALARM_URGENT_DELAY : TIMER_ALARM_REGULAR_DELAY));
	}
}

void parse_iso_timestamp(const char* source_time_stamp, struct tm* time) {
	// Expected Input: 2021-03-12T12:26:10.223-06:00
	//				   YYYY-MM-DDTHH:mm:ss.sssZ
	//				   Year-Month-DayTHour:Minute:Second:Millisecond:TimezoneOffset

	char buffer[10];

	// Year
	memcpy(buffer, &source_time_stamp[0], 4); 
	buffer[4] = '\0';
	time->tm_year = atoi(buffer) - 1900;

	// Month
	memcpy(buffer, &source_time_stamp[5], 2); 
	buffer[2] = '\0';
	time->tm_mon = atoi(buffer) - 1;	// tm struct Month is from 0 to 11 incoming month is from 1 to 12

	// Day
	memcpy(buffer, &source_time_stamp[8], 2); 
	buffer[2] = '\0';
	time->tm_mday = (atoi(buffer));

	// Hour
	memcpy(buffer, &source_time_stamp[11], 2); 
	buffer[2] = '\0';
	time->tm_hour = atoi(buffer);

	// Minute
	memcpy(buffer, &source_time_stamp[14], 2); 
	buffer[2] = '\0';
	time->tm_min = atoi(buffer);

	// Second
	memcpy(buffer, &source_time_stamp[17], 2); 
	buffer[2] = '\0';
	time->tm_sec = atoi(buffer);

	// Timezone Offest Hour from UTC
	memcpy(buffer, &source_time_stamp[strlen(source_time_stamp) - 5], 2); 
	buffer[2] = '\0';
	int hourOffest = atoi(buffer);

	// Timezone Offset Minute from UTC
	memcpy(buffer, &source_time_stamp[strlen(source_time_stamp) - 2], 2); 
	buffer[2] = '\0';
	int minuteOffest = atoi(buffer);

	time_t time_in_seconds = mktime(time);
	int totalSecondsOffset = hourOffest * 3600 + minuteOffest * 60;
	// Sign (+ or -) of Timezone Offset from UTC
	char sign = source_time_stamp[strlen(source_time_stamp) - 6];

	// Factor in timezone offset
	if(sign == '+') {
		time_in_seconds -= totalSecondsOffset;
	} else if (sign == '-') {
		time_in_seconds += totalSecondsOffset;
	} else {
		return;
	}

	memcpy(time, gmtime(&time_in_seconds), sizeof(struct tm));
}

void init_lights() {
	uint8_t on_hr, on_min, off_hr, off_min;

	if( !nvs_get_uint8(GROW_LIGHT_NVS_NAMESPACE, LIGHTS_ON_HR_KEY, &on_hr) ||
		!nvs_get_uint8(GROW_LIGHT_NVS_NAMESPACE, LIGHTS_ON_HR_KEY, &on_min) ||
		!nvs_get_uint8(GROW_LIGHT_NVS_NAMESPACE, LIGHTS_ON_HR_KEY, &off_hr) ||
		!nvs_get_uint8(GROW_LIGHT_NVS_NAMESPACE, LIGHTS_ON_HR_KEY, &off_min)) {
		ESP_LOGE("GROW_LIGHTS", "Unable to get Grow Lights settings from NVS");
		return;
	}

	struct tm time;
	get_date_time(&time);

	time.tm_hour = on_hr;
	time.tm_min = on_min;
	time.tm_sec = 0;
	enable_alarm(&day_time_alarm, time);

	time.tm_hour = off_hr;
	time.tm_min = off_min;
	enable_alarm(&night_time_alarm, time);
}

void init_irrigation() {
	is_irrigation_on = false;

	if( !nvs_get_uint32(IRRIGATION_NVS_NAMESPACE, IRRIGATION_ON_KEY, &irrigation_on_time) ||
		!nvs_get_uint32(IRRIGATION_NVS_NAMESPACE, IRRIGATION_OFF_KEY, &irrigation_off_time)) {
		ESP_LOGE("IRRIGATIOn", "Unable to get Irrigation settings from NVS");
		return;
	}

	ESP_LOGI("", "Irrigation on time: %d", irrigation_on_time);
	ESP_LOGI("", "Irrigation off time: %d", irrigation_off_time);

	enable_timer(&dev, &irrigation_timer, irrigation_off_time);
}

void irrigation_control() {
	const char *IRRIGATION_CONTROL_TAG = "IRRIGATION_CONTROL";

	if(is_irrigation_on) {
		irrigation_off();
		enable_timer(&dev, &irrigation_timer, irrigation_off_time);
		ESP_LOGI(IRRIGATION_CONTROL_TAG, "Irrigation off");
		is_irrigation_on = false;
	} else {
		irrigation_on();
		enable_timer(&dev, &irrigation_timer, irrigation_on_time);
		ESP_LOGI(IRRIGATION_CONTROL_TAG, "Irrigation on");
		is_irrigation_on = true;
	}
}

void update_irrigation_timings(cJSON *obj) {
	const char* UPDATE_IRRIGATION_KEY = "UPDATE_IRRIGATION";

	cJSON *element = obj->child;
	nvs_handle_t *handle = nvs_get_handle(IRRIGATION_NVS_NAMESPACE);

	while(element != NULL) {
		if(strcmp(element->string, IRRIGATION_ON_KEY) == 0) {
			irrigation_on_time = element->valueint;
			nvs_add_uint32(handle, IRRIGATION_ON_KEY, irrigation_on_time);
			ESP_LOGI(UPDATE_IRRIGATION_KEY, "Updated irrigation on time to: %d", irrigation_on_time);
		} else if(strcmp(element->string, IRRIGATION_OFF_KEY) == 0) {
			irrigation_off_time = element->valueint;
			nvs_add_uint32(handle, IRRIGATION_OFF_KEY, irrigation_off_time);
			ESP_LOGI(UPDATE_IRRIGATION_KEY, "Updated irrigation off time to: %d", irrigation_off_time);
		} else {
			ESP_LOGE(UPDATE_IRRIGATION_KEY, "Error: Invalid Key");
		}
		element = element->next;
	}
	nvs_commit_data(handle);
}

void update_grow_light_timings(cJSON *obj) {
	const char* UPDATE_GROW_LIGHTS_KEY = "UPDATE_GROW_LIGHTS";

	cJSON *element = obj->child;
	nvs_handle_t *handle = nvs_get_handle(GROW_LIGHT_NVS_NAMESPACE);

	while(element != NULL) {
		if(strcmp(element->string, LIGHTS_ON_KEY) == 0) {
			struct tm time;
			parse_iso_timestamp(element->valuestring, &time);
			
			nvs_add_uint8(handle, LIGHTS_ON_HR_KEY, time.tm_hour);
			nvs_add_uint8(handle, LIGHTS_ON_MIN_KEY, time.tm_min);

			enable_alarm(&day_time_alarm, time);
			ESP_LOGI("", "Lights on time: %d hr and %d min", time.tm_hour, time.tm_min);
		} else if(strcmp(element->string, LIGHTS_OFF_KEY) == 0) {
			struct tm time;
			parse_iso_timestamp(element->valuestring, &time);

			nvs_add_uint8(handle, LIGHTS_OFF_HR_KEY, time.tm_hour);
			nvs_add_uint8(handle, LIGHTS_OFF_MIN_KEY, time.tm_min);

			enable_alarm(&night_time_alarm, time);
			ESP_LOGI("", "Lights off time: %d hr and %d min", time.tm_hour, time.tm_min);
		} else {
			ESP_LOGE(UPDATE_GROW_LIGHTS_KEY, "Error: Invalid Key");
		}
		element = element->next;
	}
	nvs_commit_data(handle);
}

void irrigation_on() {
	ESP_LOGI("", "Turning irrigation on");
	control_power_outlet(IRRIGATION, true);
}
void irrigation_off() {
	ESP_LOGI("", "Turning irrigation off");
	control_power_outlet(IRRIGATION, false);
}