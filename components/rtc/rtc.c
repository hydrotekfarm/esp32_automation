#include "rtc.h"
#include <string.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"

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

	// TODO Turn lights off
	ESP_LOGI("", "Turning lights off");
}

// Enable night time routine
void night() {
	is_day = false;

	// TODO Turn lights on
	ESP_LOGI("", "Turning lights on");
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
	check_rtc_reset();

	// Water pump timings
	irrigation_on_time = 15 * 60;
	irrigation_off_time  = 45 * 60;

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
	init_timer(&reservoir_change_timer, &reservoir_change, true, false);

	// Get alarm times
	struct tm lights_on_time;
	struct tm lights_off_time;
	get_day_night_times(&lights_on_time, &lights_off_time);

	// Initialize alarms
	init_alarm(&night_time_alarm, &night, true, false);
	init_alarm(&day_time_alarm, &day, true, false);
}
void set_time() { // Set current time to some date
	// TODO Have user input for time so actual time is set
	struct tm time;

	time.tm_year = 120; // Years since 1900
	time.tm_mon = 6; // 0-11
	time.tm_mday = 7; // day of month
	time.tm_hour = 9; // 0-24
	time.tm_min = 59;
	time.tm_sec = 0;

	ESP_ERROR_CHECK(ds3231_set_time(&dev, &time));
}

void check_rtc_reset() {
	// Get current time
	struct tm time;
	ds3231_get_time(&dev, &time);

	// If year is less than 2020 (RTC was reset), set time again
	if(time.tm_year < 120) set_time();
}

void get_date_time(struct tm *time) {
	// Get current time and set it to return var
	ds3231_get_time(&dev, &(*time));

	// If year is less than 2020 (RTC was reset), set time again and set new time to return var
	if(time->tm_year < 120) {
		set_time();
		ds3231_get_time(&dev, &(*time));
	}
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

		// Check if any timer or alarm is urgent
		bool urgent = (irrigation_timer.active && irrigation_timer.high_priority) || (get_ph_control()->dose_timer.active && get_ph_control()->dose_timer.high_priority) || (get_ph_control()->wait_timer.active && get_ph_control()->wait_timer.high_priority) || (get_ec_control()->dose_timer.active && get_ec_control()->dose_timer.high_priority) || (get_ec_control()->wait_timer.active && get_ec_control()->wait_timer.high_priority) || (night_time_alarm.alarm_timer.active && night_time_alarm.alarm_timer.high_priority) || (day_time_alarm.alarm_timer.active && day_time_alarm.alarm_timer.high_priority);

		// Set priority and delay based on urgency of timers and alarms
		vTaskPrioritySet(timer_alarm_task_handle, urgent ? (configMAX_PRIORITIES - 1) : TIMER_ALARM_TASK_PRIORITY);
		vTaskDelay(pdMS_TO_TICKS(urgent ? TIMER_ALARM_URGENT_DELAY : TIMER_ALARM_REGULAR_DELAY));
	}
}

void init_irrigation() {
	is_irrigation_on = false;

	nvs_get_uint32(IRRIGATION_NVS_NAMESPACE, IRRIGATION_ON_KEY, &irrigation_on_time);
	nvs_get_uint32(IRRIGATION_NVS_NAMESPACE, IRRIGATION_OFF_KEY, &irrigation_off_time);

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

void irrigation_on() {
	ESP_LOGI("", "Turning irrigation on");
	control_power_outlet(IRRIGATION, true);
}
void irrigation_off() {
	ESP_LOGI("", "Turning irrigation off");
	control_power_outlet(IRRIGATION, false);
}