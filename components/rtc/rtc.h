#include "ds3231.h"

#include <cJSON.h>

// RTC dev
i2c_dev_t dev;

// Timer and alarm periods
#define TIMER_ALARM_URGENT_DELAY 10
#define TIMER_ALARM_REGULAR_DELAY 50

// Keys
#define LIGHTS_ON_KEY "lights_on"
#define LIGHTS_OFF_KEY "lights_off"
#define LIGHTS_ON_HR_KEY "on_hr"
#define LIGHTS_ON_MIN_KEY "on_min"
#define LIGHTS_OFF_HR_KEY "off_hr"
#define LIGHTS_OFF_MIN_KEY "off_min"

// Task handle
TaskHandle_t timer_alarm_task_handle;

// Timers -> add timers below



// Alarms
struct alarm day_time_alarm;
struct alarm night_time_alarm;

// Day or night time
bool is_day;

// Day and night times
uint32_t day_time_hour;
uint32_t day_time_min;
uint32_t night_time_hour;
uint32_t night_time_min;

// Initialize rtc
void init_rtc();

// Initialize sntp server
void init_sntp();

// Set current time
void set_time();

// Get current day and time
void get_date_time(struct tm *time);

// Timer and alarm task
void manage_timers_alarms();


