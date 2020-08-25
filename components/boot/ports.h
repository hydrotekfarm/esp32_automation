#include <esp_adc_cal.h>

// GPIO and ADC Ports
#define RF_TRANSMITTER_GPIO 4			// GPIO 4
#define ULTRASONIC_ECHO_GPIO 13			// GPIO 13
#define ULTRASONIC_TRIGGER_GPIO 16		// GPIO 16
#define TEMPERATURE_SENSOR_GPIO 17		// GPIO 17
#define PH_UP_PUMP_GPIO 18              // GPIO 18
#define PH_DOWN_PUMP_GPIO 19            // GPIO 19
#define RTC_SDA_GPIO 21                 // GPIO 21
#define RTC_SCL_GPIO 22                 // GPIO 22
#define EC_NUTRIENT_1_PUMP_GPIO 23      // GPIO 23
#define EC_NUTRIENT_2_PUMP_GPIO 25      // GPIO 25
#define EC_NUTRIENT_3_PUMP_GPIO 26      // GPIO 26
#define EC_NUTRIENT_4_PUMP_GPIO 27      // GPIO 27
#define EC_NUTRIENT_5_PUMP_GPIO 32      // GPIO 32
#define EC_NUTRIENT_6_PUMP_GPIO 33      // GPIO 33
#define EC_SENSOR_GPIO ADC_CHANNEL_0    // GPIO 36
#define PH_SENSOR_GPIO ADC_CHANNEL_3    // GPIO 39

esp_adc_cal_characteristics_t *adc_chars;  // ADC 1 Configuration Settings

