#include "rtc.h"
#include <string.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"

#include "port_manager.h"
#include "ec_control.h"
#include "ph_control.h"
#include "task_priorities.h"

void init_rtc() { // Init RTC
	memset(&dev, 0, sizeof(i2c_dev_t));
	ESP_ERROR_CHECK(ds3231_init_desc(&dev, 0, RTC_SDA_GPIO, RTC_SCL_GPIO));
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

// Trigger method for water pump timer
void water_pump() {
	const char *TAG = "Water Pump";

	// Check if pump was just on
	if(water_pump_timer.duration == water_pump_on_time) {
		// TODO Turn water pump off

		// Start pump off timer
		ESP_LOGI(TAG, "Turning pump off");
		enable_timer(&dev, &water_pump_timer, water_pump_off_time);

		// Pump was just off
	} else if(water_pump_timer.duration == water_pump_off_time) {
		// TODO Turn water pump on

		// Start pump on  timer
		ESP_LOGI(TAG, "Turning  pump on");
		enable_timer(&dev, &water_pump_timer, water_pump_on_time);
	}
}

// Turn lights on
void lights_on() {
	// TODO Turn lights on
	ESP_LOGI("", "Turning lights on");
}

// Turn lights off
void lights_off() {
	// TODO Turn lights off
	ESP_LOGI("", "Turning lights off");
}

void get_lights_times(struct tm *lights_on_time, struct tm *lights_off_time) {
	// Get current date time
	struct tm current_date_time;
	ds3231_get_time(&dev, &current_date_time);

	// Set alarm times
	lights_on_time->tm_year = current_date_time.tm_year;
	lights_on_time->tm_mon = current_date_time.tm_mon;
	lights_on_time->tm_mday = current_date_time.tm_mday;
	lights_on_time->tm_hour = lights_on_hour;
	lights_on_time->tm_min  = lights_on_min;
	lights_on_time->tm_sec = 0;

	lights_off_time->tm_year = current_date_time.tm_year;
	lights_off_time->tm_mon = current_date_time.tm_mon;
	lights_off_time->tm_mday = current_date_time.tm_mday;
	lights_off_time->tm_hour = lights_off_hour;
	lights_off_time->tm_min  = lights_off_min;
	lights_off_time->tm_sec = 0;
}

void do_nothing() {}

void manage_timers_alarms(void *parameter) {
	const char *TAG = "TIMER_TASK";

	// Water pump timings
	water_pump_on_time = 15 * 60;
	water_pump_off_time  = 45 * 60;

	// Lights
	lights_on_hour = 21;
	lights_on_min = 0;
	lights_off_hour  = 6;
	lights_off_min = 0;


	// Initialize timers
	init_timer(&water_pump_timer, &water_pump, false, false);
	init_timer(&ph_dose_timer, &ph_pump_off, false, true);
	init_timer(&ph_wait_timer, &do_nothing, false, false);
	init_timer(&ec_dose_timer, &ec_dose, false, true);
	init_timer(&ec_wait_timer, &do_nothing, false, false);

	// Get alarm times
	struct tm lights_on_time;
	struct tm lights_off_time;
	get_lights_times(&lights_on_time, &lights_off_time);

	// Initialize alarms
	init_alarm(&lights_on_alarm, &lights_on, true, false);
	init_alarm(&lights_off_alarm, &lights_off, true, false);

	ESP_LOGI(TAG, "Timers initialized");

	for(;;) {
		// Get current unix time
		time_t unix_time;
		get_unix_time(&dev, &unix_time);

		// Check if timers are done
		check_timer(&dev, &water_pump_timer, unix_time);
		check_timer(&dev, &ph_dose_timer, unix_time);
		check_timer(&dev, &ph_wait_timer, unix_time);
		check_timer(&dev, &ec_dose_timer, unix_time);
		check_timer(&dev, &ec_wait_timer, unix_time);

		// Check if alarms are done
		check_alarm(&dev, &lights_on_alarm, unix_time);
		check_alarm(&dev, &lights_off_alarm, unix_time);

		// Check if any timer or alarm is urgent
		bool urgent = (water_pump_timer.active && water_pump_timer.high_priority) || (ph_dose_timer.active && ph_dose_timer.high_priority) || (ph_wait_timer.active && ph_wait_timer.high_priority) || (ec_dose_timer.active && ec_dose_timer.high_priority) || (ec_wait_timer.active && ec_wait_timer.high_priority) || (lights_on_alarm.alarm_timer.active && lights_on_alarm.alarm_timer.high_priority) || (lights_off_alarm.alarm_timer.active && lights_off_alarm.alarm_timer.high_priority);

		// Set priority and delay based on urgency of timers and alarms
		vTaskPrioritySet(timer_alarm_task_handle, urgent ? (configMAX_PRIORITIES - 1) : TIMER_ALARM_TASK_PRIORITY);
		vTaskDelay(pdMS_TO_TICKS(urgent ? TIMER_ALARM_URGENT_DELAY : TIMER_ALARM_REGULAR_DELAY));
	}
}
