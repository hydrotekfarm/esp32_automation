#include "test_hardware.h"
#include "ports.h"
#include "rf_lib.h"
#include "rf_transmitter.h"
#include "ph_sensor.h"
#include "ec_sensor.h"
#include "ds18x20.h"
#include "esp_log.h"


#define DEVICE_ON 1
#define DEVICE_OFF 0
#define DEVICE_ERROR -1

ph_sensor_t ph_dev;
ec_sensor_t ec_dev;

ds18x20_addr_t ds18b20_address[1];

bool is_mcp23017 = false;
bool is_rf = false;
bool is_ph = false;
bool is_ec = true;
bool is_water_temperature = false;
   
void test_hardware() {
    printf("\n\n");
    ESP_LOGI("TEST_HARDWARE", "Testing Hardware");
    gpio_set_direction(GREEN_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(GREEN_LED, 1);
    
	ESP_ERROR_CHECK(i2cdev_init()); // Init i2cdev
    if(is_mcp23017) init_ports();
    if(is_rf) init_rf_protocol();
    if(is_ph) init_ph();
    if(is_ec) init_ec();
    if(is_water_temperature) init_water_temperature();

    if(is_mcp23017) test_mcp23017();
    if(is_rf) test_rf();
    if(is_ph) test_ph();
    if(is_ec) test_ec();
    if(is_water_temperature) test_water_temperature();

    printf("\n");
    ESP_LOGI("TEST_HARDWARE", "Testing Hardware Complete");
}

void test_mcp23017() {
    printf("\n");
    ESP_LOGI("MCP_23017_TEST", "Testing MCP_23017");
    printf("-------------------------------------------------\n");
    ESP_LOGI("MCP_23017_TEST", "Turning Pumps On");
    set_gpio_on(PH_DOWN_PUMP_GPIO);
    set_gpio_on(PH_UP_PUMP_GPIO);
    set_gpio_on(EC_NUTRIENT_1_PUMP_GPIO);
    set_gpio_on(EC_NUTRIENT_2_PUMP_GPIO);
    set_gpio_on(EC_NUTRIENT_3_PUMP_GPIO);
    set_gpio_on(EC_NUTRIENT_4_PUMP_GPIO);
    set_gpio_on(EC_NUTRIENT_5_PUMP_GPIO);
    set_gpio_on(EC_NUTRIENT_6_PUMP_GPIO);

    vTaskDelay(pdMS_TO_TICKS(5000));

    ESP_LOGI("MCP_23017_TEST", "Turning Pumps Off");
    set_gpio_off(PH_DOWN_PUMP_GPIO);
    set_gpio_off(PH_UP_PUMP_GPIO);
    set_gpio_off(EC_NUTRIENT_1_PUMP_GPIO);
    set_gpio_off(EC_NUTRIENT_2_PUMP_GPIO);
    set_gpio_off(EC_NUTRIENT_3_PUMP_GPIO);
    set_gpio_off(EC_NUTRIENT_4_PUMP_GPIO);
    set_gpio_off(EC_NUTRIENT_5_PUMP_GPIO);
    set_gpio_off(EC_NUTRIENT_6_PUMP_GPIO);
}

void init_ph() {
	memset(&ph_dev, 0, sizeof(ph_sensor_t));
	ESP_ERROR_CHECK(ph_init(&ph_dev, 0, PH_ADDR_BASE, SDA_GPIO, SCL_GPIO)); // Initialize PH I2C communication
    ESP_ERROR_CHECK(activate_ph(&ph_dev));
}

void init_ec() {
    memset(&ec_dev, 0, sizeof(ec_sensor_t));
	ESP_ERROR_CHECK(ec_init(&ec_dev, 0, EC_ADDR_BASE, SDA_GPIO, SCL_GPIO)); // Initialize EC I2C communication
    ESP_ERROR_CHECK(activate_ec(&ec_dev));
}

void init_water_temperature() {
	// Scan and setup sensor
    gpio_config_t temperature_gpio_config = { GPIO_SEL_14, GPIO_MODE_OUTPUT };
    gpio_config(&temperature_gpio_config);
    
	uint32_t sensor_count = ds18x20_scan_devices(TEMPERATURE_SENSOR_GPIO, ds18b20_address, 1);

	sensor_count = ds18x20_scan_devices(TEMPERATURE_SENSOR_GPIO, ds18b20_address, 1);
	vTaskDelay(pdMS_TO_TICKS(1000));

	if(sensor_count < 1) ESP_LOGE("WATER_TEMPERATURE_TEST", "Sensor Not Found");
}

void test_rf() {
    printf("\n");
    ESP_LOGI("RF_TEST", "Testing RF Transmitter");
    printf("-------------------------------------------------\n");
    // address: any 20 digit binary, state (on/off): (0011, 1100)
    for(int i = 0; i < 5; i++) {
        ESP_LOGI("RF_TEST", "Turning Power Outlet On");
        send_message("00010100010101010011", "0011");
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        ESP_LOGI("RF_TEST", "Turning Power Outlet Off");
        send_message("00010100010101010011", "1100");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void test_ph() {
    printf("\n");
    ESP_LOGI("PH_TEST", "Testing pH Sensor");
    printf("-------------------------------------------------\n");
    clear_calibration_ph(&ph_dev);
    for(int i = 0; i < 10; i++) {
        float ph_reading = 0;
        read_ph(&ph_dev, &ph_reading);
        ESP_LOGI("PH_TEST", "pH Reading: %f", ph_reading);
        vTaskDelay(pdMS_TO_TICKS(1500));
    }
//    calibrate_ph(&ph_dev, 24);

    for(;;) {
        float ph_reading = 0;
        read_ph(&ph_dev, &ph_reading);
        ESP_LOGI("PH_TEST", "pH Reading: %f", ph_reading);
    }
}

void test_ec() {
    printf("\n");
    ESP_LOGI("EC_TEST", "Testing EC Sensor");
    printf("-------------------------------------------------\n");
    for(int i = 0; i < 5; i++) {
        float ec_reading = 0;
        read_ec(&ec_dev, &ec_reading);
        ESP_LOGI("EC_TEST", "EC Reading: %f", ec_reading);
    }
}

void test_water_temperature() {
    for (int i = 0; i < 5; i++) {
        float water_temperature_reading = 0;
		// Perform Temperature Calculation and Read Temperature; vTaskDelay in the source code of this function
		esp_err_t error = ds18x20_measure_and_read(TEMPERATURE_SENSOR_GPIO, ds18b20_address[0], &water_temperature_reading);
	
		if (error == ESP_OK) {
			ESP_LOGI("WATER_TEMPERATURE_TEST", "temperature: %f\n", water_temperature_reading);
		} else if (error == ESP_ERR_INVALID_RESPONSE) {
			ESP_LOGE("WATER_TEMPERATURE_TEST", "Temperature Sensor Not Connected\n");
		} else if (error == ESP_ERR_INVALID_CRC) {
			ESP_LOGE("WATER_TEMPERATURE_TEST", "Invalid CRC, Try Again\n");
		} else {
			ESP_LOGE("WATER_TEMPERATURE_TEST", "Unknown Error\n");
		}
	}
}

void test_motor(int motor_choice, int motor_status)
{
    const char *TAG = "TEST_MOTOR";

    printf("\n");
    ESP_LOGI(TAG, "Testing the motor");
    printf("-------------------------------------------------\n");
    switch (motor_choice)
    {
    case 1:
        ESP_LOGI(TAG, "This is Motor_1");
        if (motor_status == DEVICE_ON)
        {
            if (set_gpio_on(EC_NUTRIENT_1_PUMP_GPIO) == ESP_OK)
            {
                publish_pump_status(motor_choice, DEVICE_ON);
            }
            else
            {
                publish_pump_status(motor_choice, DEVICE_ERROR);
            }
        }
        else
            set_gpio_off(EC_NUTRIENT_1_PUMP_GPIO);
        publish_pump_status(motor_choice, DEVICE_OFF);
        break;

    case 2:
        ESP_LOGI(TAG, "This is Motor_2");
        if (motor_status == DEVICE_ON)
        {
            if (set_gpio_on(EC_NUTRIENT_1_PUMP_GPIO) == ESP_OK)
            {
                publish_pump_status(motor_choice, DEVICE_ON);
            }
            else
            {
                publish_pump_status(motor_choice, DEVICE_ERROR);
            }
        }
        else
        {
            set_gpio_off(EC_NUTRIENT_1_PUMP_GPIO);
            publish_pump_status(motor_choice, DEVICE_OFF);
        }
        break;

    case 3:
        ESP_LOGI(TAG, "This is Motor_3");
        if (motor_status == DEVICE_ON)
        {
            if (set_gpio_on(EC_NUTRIENT_1_PUMP_GPIO) == ESP_OK)
            {
                publish_pump_status(motor_choice, DEVICE_ON);
            }
            else
            {
                publish_pump_status(motor_choice, DEVICE_ERROR);
            }
        }
        else
        {
            set_gpio_off(EC_NUTRIENT_1_PUMP_GPIO);
            publish_pump_status(motor_choice, DEVICE_OFF);
        }
        break;

    case 4:
        ESP_LOGI(TAG, "This is Motor_4");
        if (motor_status == DEVICE_ON)
        {
            if (set_gpio_on(EC_NUTRIENT_1_PUMP_GPIO) == ESP_OK)
            {
                publish_pump_status(motor_choice, DEVICE_ON);
            }
            else
            {
                publish_pump_status(motor_choice, DEVICE_ERROR);
            }
        }
        else
        {
            set_gpio_off(EC_NUTRIENT_1_PUMP_GPIO);
            publish_pump_status(motor_choice, DEVICE_OFF);
        }
        break;
    case 5:
        ESP_LOGI(TAG, "This is Motor_5");
        if (motor_status == DEVICE_ON)
        {
            if (set_gpio_on(EC_NUTRIENT_1_PUMP_GPIO) == ESP_OK)
            {
                publish_pump_status(motor_choice, DEVICE_ON);
            }
            else
            {
                publish_pump_status(motor_choice, DEVICE_ERROR);
            }
        }
        else
        {
            set_gpio_off(EC_NUTRIENT_1_PUMP_GPIO);
            publish_pump_status(motor_choice, DEVICE_OFF);
        }
        break;

    default:
        ESP_LOGI(TAG, "There is no such motor");
        break;
    }
}

void test_lights(int light_choice, int light_status)
{
    const char *TAG = "TEST_LIGHTS";
    printf("\n");
    ESP_LOGI(TAG, "Testing the lights");
    printf("-------------------------------------------------\n");
    switch (light_choice)
    {
    case 1:
        ESP_LOGI(TAG, "This is Light_1");
        if (light_status == DEVICE_ON)
        {
            if (set_gpio_on(EC_NUTRIENT_1_PUMP_GPIO) == ESP_OK)
            {
                publish_light_status(light_choice, DEVICE_ON);
            }
            else
            {
                publish_light_status(light_choice, DEVICE_ERROR);
            }
        }
        else
        {
            set_gpio_off(EC_NUTRIENT_1_PUMP_GPIO);
            publish_light_status(light_choice, DEVICE_OFF);
        }
        break;

    case 2:
        ESP_LOGI(TAG, "This is Light_2");
        if (light_status == DEVICE_ON)
        {
            if (set_gpio_on(EC_NUTRIENT_1_PUMP_GPIO) == ESP_OK)
            {
                publish_light_status(light_choice, DEVICE_ON);
            }
            else
            {
                publish_light_status(light_choice, DEVICE_ERROR);
            }
        }
        else
        {
            set_gpio_off(EC_NUTRIENT_1_PUMP_GPIO);
            publish_light_status(light_choice, DEVICE_OFF);
        }
        break;

    case 3:
        ESP_LOGI(TAG, "This is Light_3");
        if (light_status == DEVICE_ON)
        {
            if (set_gpio_on(EC_NUTRIENT_1_PUMP_GPIO) == ESP_OK)
            {
                publish_light_status(light_choice, DEVICE_ON);
            }
            else
            {
                publish_light_status(light_choice, DEVICE_ERROR);
            }
        }
        else
        {
            set_gpio_off(EC_NUTRIENT_1_PUMP_GPIO);
            publish_light_status(light_choice, DEVICE_OFF);
        }
        break;
    case 4:
        ESP_LOGI(TAG, "This is Light_4");
        if (light_status == DEVICE_ON)
        {
            if (set_gpio_on(EC_NUTRIENT_1_PUMP_GPIO) == ESP_OK)
            {
                publish_light_status(light_choice, DEVICE_ON);
            }
            else
            {
                publish_pump_status(light_choice, DEVICE_ERROR);
            }
        }
        else
        {
            set_gpio_off(EC_NUTRIENT_1_PUMP_GPIO);
            publish_light_status(light_choice, DEVICE_OFF);
        }
        break;

    case 5:
        ESP_LOGI(TAG, "This is Light_5");
        if (light_status == DEVICE_ON)
        {
            if (set_gpio_on(EC_NUTRIENT_1_PUMP_GPIO) == ESP_OK)
            {
                publish_light_status(light_choice, DEVICE_ON);
            }
            else
            {
                publish_light_status(light_choice, DEVICE_ERROR);
            }
        }
        else
        {
            set_gpio_off(EC_NUTRIENT_1_PUMP_GPIO);
            publish_light_status(light_choice, DEVICE_OFF);
        }
        break;

    default:
        ESP_LOGI(TAG, "There is no such light");
        break;
    }
}
