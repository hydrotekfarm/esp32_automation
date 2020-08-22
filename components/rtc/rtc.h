#include "ds3231.h"

// RTC dev
i2c_dev_t dev;

// Timer and alarm periods
static const uint32_t timer_alarm_urgent_delay = 10;
static const uint32_t timer_alarm_regular_delay = 50;

// Timers
struct timer water_pump_timer;
struct timer ph_dose_timer;
struct timer ph_wait_timer;
struct timer ec_dose_timer;
struct timer ec_wait_timer;

// Alarms
struct alarm lights_on_alarm;
struct alarm lights_off_alarm;

// Water pump timings
static uint32_t water_pump_on_time = 15 * 60;
static uint32_t water_pump_off_time  = 45 * 60;

// Lights
static uint32_t lights_on_hour = 21;
static uint32_t lights_on_min = 0;
static uint32_t lights_off_hour  = 6;
static uint32_t lights_off_min = 0;

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
