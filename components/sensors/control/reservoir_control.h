#include <stdbool.h>
#include "rf_transmitter.h"
#include "time.h"
#include "rtc.h"

#define RESERVOIR_REPLACEMENT_INTERVAL_KEY "replace_interv"
#define RESERVOIR_ENABLED_KEY "is_control"
#define RESERVOIR_NEXT_REPLACEMENT_DATE_KEY "replace_date"

bool reservoir_control_active;
bool reservoir_change_flag;
bool top_float_switch_trigger;
bool bottom_float_switch_trigger;
uint16_t reservoir_replacement_interval;

struct rf_message water_in_rf_message;
struct rf_message water_out_rf_message;

struct alarm reservoir_replacement_alarm;

struct tm next_replacement_date;

void set_reservoir_change_flag(bool active);

void check_water_level();

void get_reservoir_nvs_settings();

struct alarm* get_reservoir_alarm();

void replace_reservoir();

void update_reservoir_settings(cJSON* obj);

void init_reservoir();
