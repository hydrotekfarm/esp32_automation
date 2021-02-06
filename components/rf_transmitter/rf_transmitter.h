#include "rf_lib.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#define RF_CODE_LENGTH 24
#define RF_ADDRESS_LENGTH 20
#define MAX_GROW_LIGHT_ZONES 10
#define DEFAULT_ADDRESS_INDEX 100000

#define RF_BASE_ADDRESS_WATER_COOLER 0
#define	RF_BASE_ADDRESS_WATER_HEATER 1
#define	RF_BASE_ADDRESS_WATER_IN 2
#define	RF_BASE_ADDRESS_WATER_OUT 3
#define RF_BASE_ADDRESS_GROW_LIGHTS 15

#define POWER_OUTLET_ON 1
#define	POWER_OUTLET_OFF 0

#ifndef RF_TRANSMITTER_H_
#define RF_TRANSMITTER_H_
static const char on_binary_code[] = "0011";
static const char off_binary_code[] = "1100";

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

char grow_light_address[RF_ADDRESS_LENGTH + 1][MAX_GROW_LIGHT_ZONES];

TaskHandle_t rf_transmitter_task_handle;
QueueHandle_t rf_transmitter_queue;

// Initialize rf bits
void init_rf();

// Send rf message
void send_rf_transmission();

// RF Task
void rf_transmitter();


