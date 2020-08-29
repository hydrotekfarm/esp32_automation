#include <esp_adc_cal.h>

// GPIO and ADC Ports
#define RF_TRANSMITTER_GPIO 4			// GPIO 4
#define ULTRASONIC_ECHO_GPIO 13			// GPIO 13
#define ULTRASONIC_TRIGGER_GPIO 16		// GPIO 16
#define TEMPERATURE_SENSOR_GPIO 17		// GPIO 17
#define INTA_GPIO 19					// GPIO 19
#define SDA_GPIO 21                 	// GPIO 21
#define SCL_GPIO 22                 	// GPIO 22
#define EC_SENSOR_GPIO ADC_CHANNEL_0    // GPIO 36
#define PH_SENSOR_GPIO ADC_CHANNEL_3    // GPIO 39


// GPIO Expansion Ports
#define EC_NUTRIENT_1_PUMP_GPIO 0      // GPIO 0
#define EC_NUTRIENT_2_PUMP_GPIO 1      // GPIO 1
#define EC_NUTRIENT_3_PUMP_GPIO 2      // GPIO 2
#define EC_NUTRIENT_4_PUMP_GPIO 3      // GPIO 3
#define EC_NUTRIENT_5_PUMP_GPIO 4      // GPIO 4
#define EC_NUTRIENT_6_PUMP_GPIO 5      // GPIO 5
#define PH_UP_PUMP_GPIO 6              // GPIO 6
#define PH_DOWN_PUMP_GPIO 7            // GPIO 7


esp_adc_cal_characteristics_t *adc_chars;  // ADC 1 Configuration Settings

