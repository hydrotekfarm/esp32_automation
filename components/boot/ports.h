#include <string.h>
#include "mcp23x17.h"

// GPIO Ports
#define POWER_BUTTON_GPIO			35
#define HARD_RESET_GPIO             33
#define RF_TRANSMITTER_GPIO 		4
#define FLOAT_SWITCH_BOTTOM_GPIO	13
#define TEMPERATURE_SENSOR_GPIO 	14
#define INTA_GPIO 					19
#define SDA_GPIO 					21
#define SCL_GPIO 					22
#define FLOAT_SWITCH_TOP_GPIO 		32
#define BLUE_LED                    25 // wifi
#define GREEN_LED                   26



// MCP23017 GPIO Expansion Ports
#define EC_NUTRIENT_1_PUMP_GPIO 	5
#define EC_NUTRIENT_2_PUMP_GPIO 	4
#define EC_NUTRIENT_3_PUMP_GPIO 	3
#define EC_NUTRIENT_4_PUMP_GPIO 	2
#define EC_NUTRIENT_5_PUMP_GPIO 	1
#define EC_NUTRIENT_6_PUMP_GPIO 	0
#define PH_UP_PUMP_GPIO 			6
#define PH_DOWN_PUMP_GPIO 			7

mcp23x17_t ports_dev;

// Initialize ports
void init_ports();

// Set gpio on and off
esp_err_t set_gpio_on(int gpio);
esp_err_t set_gpio_off(int gpio);
