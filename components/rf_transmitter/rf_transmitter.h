#include "rf_lib.h"
#include <Freertos/freertos.h>
#include <Freertos/task.h>
#include <Freertos/queue.h>

#define RF_CODE_LENGTH 24
#define RF_ADDRESS_LENGTH 20
#define MAX_GROW_LIGHT_ZONES 10
#define NUM_OUTLETS MAX_GROW_LIGHT_ZONES+5
#define DEFAULT_ADDRESS_INDEX 100000

#define POWER_OUTLET_ON 1
#define	POWER_OUTLET_OFF 0

#ifndef RF_TRANSMITTER_H_
#define RF_TRANSMITTER_H_
static const char on_binary_code[] = "0011";
static const char off_binary_code[] = "1100";

#define RF_TAG "RF_TRANSMITTER"

enum power_outlets {
	WATER_COOLER,
	WATER_HEATER,
	IRRIGATION,
	RESERVOIR_WATER_IN,
	RESERVOIR_WATER_OUT,
	GROW_LIGHTS
};

struct rf_message {
	char* rf_address_ptr;
	bool state;
};
#endif

uint32_t address_index;
uint8_t grow_light_arr_current_length;

char water_cooler_address[RF_ADDRESS_LENGTH + 1];
char water_heater_address[RF_ADDRESS_LENGTH + 1];

char water_in_address[RF_ADDRESS_LENGTH + 1];
char water_out_address[RF_ADDRESS_LENGTH + 1];

char irrigation_address[RF_ADDRESS_LENGTH + 1];

char grow_lights_address[MAX_GROW_LIGHT_ZONES][RF_ADDRESS_LENGTH + 1];

TaskHandle_t rf_transmitter_task_handle;
QueueHandle_t rf_transmitter_queue;

// Initialize rf bits
void init_rf();

// Send rf message
esp_err_t control_power_outlet(int power_outlet_id, bool state);

// RF Task
void rf_transmitter();

// Turn all lights on
void lights_on();

// Turn all lights off
void lights_off();
