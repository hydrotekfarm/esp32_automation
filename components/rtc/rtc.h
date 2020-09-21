#include "ds3231.h"

// RTC dev
i2c_dev_t dev;

// Timer and alarm periods
#define TIMER_ALARM_URGENT_DELAY 10
#define TIMER_ALARM_REGULAR_DELAY 50

// Task handle
TaskHandle_t timer_alarm_task_handle;

// Timers
struct timer water_pump_timer;
struct timer ph_dose_timer;
struct timer ph_wait_timer;
struct timer ec_dose_timer;
struct timer ec_wait_timer;

// Alarms
struct alarm day_time_alarm;
struct alarm night_time_alarm;

// Water pump timings
uint32_t water_pump_on_time;
uint32_t water_pump_off_time;

// Day or night time
bool is_day;

// Day and night times
uint32_t day_time_hour;
uint32_t day_time_min;
uint32_t night_time_hour;
uint32_t night_time_min;

// Initialize rtc
void init_rtc();

// Set current time
void set_time();

// Check if rtc needs to be reset
void check_rtc_reset();

// Get current day and time
void get_date_time(struct tm *time);

// Timer and alarm task
void manage_timers_alarms();
