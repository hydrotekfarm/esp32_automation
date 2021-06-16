#include "rf_lib.h"
#include <Freertos/freertos.h>
#include <Freertos/task.h>
#include <Freertos/queue.h>

#define RF_CODE_LENGTH 24
#define RF_ADDRESS_LENGTH 20
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
	CO2_INJECTION,
	HUMIDIFIER_INCREASE,
	HUMIDIFIER_DECREASE,
	TEMPERATURE_INCREASE,
	TEMPERATURE_DECREASE
};

struct rf_message {
	char* rf_address_ptr;
	bool state;
};
#endif

uint32_t address_index;
uint8_t grow_light_arr_current_length;


char co2_injector_address[RF_ADDRESS_LENGTH + 1];

char temp_inc_address[RF_ADDRESS_LENGTH + 1];
char temp_dec_address[RF_ADDRESS_LENGTH + 1];

char humidifier_inc_address[RF_ADDRESS_LENGTH + 1];
char humidifier_dec_address[RF_ADDRESS_LENGTH + 1];

TaskHandle_t rf_transmitter_task_handle;
QueueHandle_t rf_transmitter_queue;

// Initialize rf bits
void init_rf();

// Send rf message
esp_err_t control_power_outlet(int power_outlet_id, bool state);

// RF Task
void rf_transmitter();
