#include <string.h>
#include "mcp23x17.h"

// GPIO and ADC Ports
#define RF_TRANSMITTER_GPIO 4			// GPIO 4
#define TEMPERATURE_SENSOR_GPIO 17		// GPIO 17
#define INTA_GPIO 19					// GPIO 19
#define SDA_GPIO 21                 	// GPIO 21
#define SCL_GPIO 22                 	// GPIO 22
#define EC_SENSOR_GPIO ADC_CHANNEL_0    // GPIO 36
#define PH_SENSOR_GPIO ADC_CHANNEL_3    // GPIO 39
#define FLOAT_SWITCH_TOP_GPIO 32		// GPIO 32
#define FLOAT_SWITCH_BOTTOM_GPIO 13		// GPIO 13


// GPIO Expansion Ports
#define EC_NUTRIENT_1_PUMP_GPIO 5     // GPIO 0
#define EC_NUTRIENT_2_PUMP_GPIO 4      // GPIO 1
#define EC_NUTRIENT_3_PUMP_GPIO 3      // GPIO 2
#define EC_NUTRIENT_4_PUMP_GPIO 2      // GPIO 3
#define EC_NUTRIENT_5_PUMP_GPIO 1      // GPIO 4
#define EC_NUTRIENT_6_PUMP_GPIO 0      // GPIO 5
#define PH_UP_PUMP_GPIO 6              // GPIO 6
#define PH_DOWN_PUMP_GPIO 7            // GPIO 7

mcp23x17_t ports_dev;

// Initialize ports
void init_ports();

// Set gpio on and off
void set_gpio_on(int gpio);
void set_gpio_off(int gpio);
