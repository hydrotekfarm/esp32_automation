#include "test_hardware.h"
#include "ports.h"
#include "rf_lib.h"
#include "rf_transmitter.h"
#include "ph_sensor.h"
#include "ec_sensor.h"
#include "ds18x20.h"
#include "esp_log.h"

void publish_pump_status(int publish_motor_choice, int publish_status);
void publish_power_outlet_status(int outlet_choice, int outlet_status);
void publish_float_switch_status(int float_switch_choice, int float_switch_status);
void publish_sensor_status(char sensor_choice[], int sensor_status);
void IRAM_ATTR top_float_switch_isr_handler(void *arg);
void IRAM_ATTR bottom_float_switch_isr_handler(void *arg);

ph_sensor_t ph_dev;
ec_sensor_t ec_dev;

ds18x20_addr_t ds18b20_address[1];

bool is_mcp23017 = false;
bool is_rf = false;
bool is_ph = false;
bool is_ec = true;
bool is_water_temperature = false;
bool is_motor = false;
bool is_lights = false;
bool is_water_cooler = false;
bool is_water_heater = false;
bool is_water_in = false;
bool is_water_out = false;
bool is_irrigation = false;
bool is_float_switch = false;

void test_hardware() {
    printf("\n\n");
    ESP_LOGI("TEST_HARDWARE", "Testing Hardware");
    gpio_set_direction(GREEN_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(GREEN_LED, 1);

    ESP_ERROR_CHECK(i2cdev_init()); // Init i2cdev
    if (is_mcp23017)
        init_ports();
    if (is_rf)
        init_rf_protocol();
    if (is_ph)
        init_ph();
    if (is_ec)
        init_ec();
    if (is_water_temperature)
        init_water_temperature();

    if (is_mcp23017)
        test_mcp23017();
    if (is_rf)
        test_rf();

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
    gpio_config_t temperature_gpio_config = {GPIO_SEL_14, GPIO_MODE_OUTPUT};
    gpio_config(&temperature_gpio_config);

    uint32_t sensor_count = ds18x20_scan_devices(TEMPERATURE_SENSOR_GPIO, ds18b20_address, 1);

    sensor_count = ds18x20_scan_devices(TEMPERATURE_SENSOR_GPIO, ds18b20_address, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));

    if (sensor_count < 1)
        ESP_LOGE("WATER_TEMPERATURE_TEST", "Sensor Not Found");
}

void test_rf() {
    printf("\n");
    ESP_LOGI("RF_TEST", "Testing RF Transmitter");
    printf("-------------------------------------------------\n");
    // address: any 20 digit binary, state (on/off): (0011, 1100)
    for (int i = 0; i < 5; i++) {
        ESP_LOGI("RF_TEST", "Turning Power Outlet On");
        send_message("00010100010101010011", "0011");
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI("RF_TEST", "Turning Power Outlet Off");
        send_message("00010100010101010011", "1100");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void test_motor(int choice, int switch_status) {
    const char *TAG = "TEST_MOTOR";
    printf("\n");
    ESP_LOGI(TAG, "Testing the motor");
    printf("-------------------------------------------------\n");
    int motor[5];
    motor[0] = EC_NUTRIENT_1_PUMP_GPIO;
    motor[1] = EC_NUTRIENT_2_PUMP_GPIO;
    motor[2] = EC_NUTRIENT_3_PUMP_GPIO;
    motor[3] = EC_NUTRIENT_4_PUMP_GPIO;
    motor[4] = EC_NUTRIENT_5_PUMP_GPIO;

    ESP_LOGI(TAG, "This is Motor_%d", choice);
    if (switch_status == DEVICE_ON) {
        if (set_gpio_on(motor[(choice)-1]) == ESP_OK){
            publish_pump_status(choice, DEVICE_ON);
        }
        else {
            publish_pump_status(choice, DEVICE_ERROR);
        }
    }
    else {
        if (set_gpio_off(motor[(choice)-1]) == ESP_OK) {
            publish_pump_status(choice, DEVICE_OFF);
        }
        else {
            publish_pump_status(choice, DEVICE_ERROR);
        }
    }
}

void test_outlet(int choice, int switch_status) {
    const char *TAG = "TEST_OUTLET";
    printf("\n");
    ESP_LOGI(TAG, "Testing the outlet");
    printf("-------------------------------------------------\n");
    enum power_outlets{
        WATER_COOLER ,
        WATER_HEATER,
        IRRIGATION,
        RESERVOIR_WATER_IN,
        RESERVOIR_WATER_OUT,
        GROW_LIGHTS
    };
    if (choice == WATER_COOLER) {
        const char *TAG = "TEST_WATER_COOLER";
        printf("\n");
        ESP_LOGI(TAG, "Testing water cooler");
        printf("-------------------------------------------------\n");
        char *water_cooler = water_cooler_address;

        struct rf_message rf_water_cooler;
        rf_water_cooler.state = switch_status;
        int result = xQueueSend(rf_transmitter_queue, &rf_water_cooler, pdMS_TO_TICKS(20000));

        if (result == pdTRUE) {
            publish_power_outlet_status(choice, switch_status);
        }
        else {
            publish_power_outlet_status(choice, DEVICE_ERROR);
        }
    }
    else if (choice == WATER_HEATER) {
        const char *TAG = "TEST_WATER_HEATER";
        printf("\n");
        ESP_LOGI(TAG, "Testing water heater");
        printf("-------------------------------------------------\n");
        char *water_heater = water_heater_address;

        struct rf_message rf_water_heater;
        rf_water_heater.state = switch_status;
        int result = xQueueSend(rf_transmitter_queue, &rf_water_heater, pdMS_TO_TICKS(20000));

        if (result == pdTRUE) {
            publish_power_outlet_status(choice, switch_status);
        }
        else {
            publish_power_outlet_status(choice, DEVICE_ERROR);
        }
    }
    else if (choice == IRRIGATION) {
        const char *TAG = "TEST_IRRIGATION";
        printf("\n");
        ESP_LOGI(TAG, "Testing irrigation");
        printf("-------------------------------------------------\n");
        char *irrigation = irrigation_address;

        struct rf_message rf_irrigation;
        rf_irrigation.state = switch_status;
        int result = xQueueSend(rf_transmitter_queue, &rf_irrigation, pdMS_TO_TICKS(20000));

        if (result == pdTRUE) {
            publish_power_outlet_status(choice, switch_status);
        }
        else {
            publish_power_outlet_status(choice, DEVICE_ERROR);
        }
    }
    else if (choice == RESERVOIR_WATER_IN) {
        const char *TAG = "TEST_WATER_IN";
        printf("\n");
        ESP_LOGI(TAG, "Testing water in");
        printf("-------------------------------------------------\n");
        char *water_in = water_in_address;

        struct rf_message rf_water_in;
        rf_water_in.state = switch_status;
        int result = xQueueSend(rf_transmitter_queue, &rf_water_in, pdMS_TO_TICKS(20000));

        if (result == pdTRUE) {
            publish_power_outlet_status(choice, switch_status);
        }
        else {
            publish_power_outlet_status(choice, DEVICE_ERROR);
        }
    }
    else if (choice == RESERVOIR_WATER_OUT) {
        const char *TAG = "TEST_WATER_OUT";
        printf("\n");
        ESP_LOGI(TAG, "Testing water out");
        printf("-------------------------------------------------\n");
        char *water_out = water_out_address;

        struct rf_message rf_water_out;
        rf_water_out.state = switch_status;
        int result = xQueueSend(rf_transmitter_queue, &rf_water_out, pdMS_TO_TICKS(20000));

        if (result == pdTRUE) {
            publish_power_outlet_status(choice, switch_status);
        }
        else {
            publish_power_outlet_status(choice, DEVICE_ERROR);
        }
    }
    else if (choice == GROW_LIGHTS) {
        const char *TAG = "TEST_LIGHTS";
        printf("\n");
        ESP_LOGI(TAG, "Testing the lights");
        printf("-------------------------------------------------\n");

        char *light_address = grow_lights_address[choice];

        struct rf_message rf_msg;
        rf_msg.rf_address_ptr = light_address;
        rf_msg.state = switch_status;
        int result = xQueueSend(rf_transmitter_queue, &rf_msg, pdMS_TO_TICKS(20000));

        if (result == pdTRUE) {
            publish_power_outlet_status(choice, switch_status);
        }
        else {
            publish_power_outlet_status(choice, DEVICE_ERROR);
        }
    }
}

void test_sensor( char choice[], int switch_status) {
    const char *TAG = "TEST_SENSOR";
    printf("\n");
    ESP_LOGI(TAG, "Testing the sensors");
    printf("-------------------------------------------------\n");

    
    if (strcmp(choice, "ph") == 0) {
        printf("\n");
        ESP_LOGI("PH_TEST", "Testing pH Sensor");
        printf("-------------------------------------------------\n");
        clear_calibration_ph(&ph_dev);
        for (int i = 0; i < 5; i++) {
            float ph_reading = 0;
            read_ph(&ph_dev, &ph_reading);
            ESP_LOGI("PH_TEST", "pH Reading: %f", ph_reading);
            publish_sensor_status(choice, switch_status);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
    else if (strcmp(choice, "ec") == 0) {
        printf("\n");
        ESP_LOGI("EC_TEST", "Testing EC Sensor");
        printf("-------------------------------------------------\n");
        for (int i = 0; i < 5; i++) {
            float ec_reading = 0;
            read_ec(&ec_dev, &ec_reading);
            ESP_LOGI("EC_TEST", "EC Reading: %f", ec_reading);
            publish_sensor_status(choice, switch_status);
        }
    }
    else if (strcmp(choice, "water_temp") == 0) {
        for (int i = 0; i < 5; i++) {
            float water_temperature_reading = 0;
            // Perform Temperature Calculation and Read Temperature; vTaskDelay in the source code of this function
            esp_err_t error = ds18x20_measure_and_read(TEMPERATURE_SENSOR_GPIO, ds18b20_address[0], &water_temperature_reading);

            if (error == ESP_OK) {
                ESP_LOGI("WATER_TEMPERATURE_TEST", "temperature: %f\n", water_temperature_reading);
                publish_sensor_status(choice, DEVICE_ON);
                break;
            }
            else if (error == ESP_ERR_INVALID_RESPONSE) {
                ESP_LOGE("WATER_TEMPERATURE_TEST", "Temperature Sensor Not Connected\n");
                publish_sensor_status(choice, DEVICE_ERROR);
                break;
            }
            else if (error == ESP_ERR_INVALID_CRC) {
                ESP_LOGE("WATER_TEMPERATURE_TEST", "Invalid CRC, Try Again\n");
                publish_sensor_status(choice, DEVICE_ERROR);
                break;
            }
            else {
                ESP_LOGE("WATER_TEMPERATURE_TEST", "Unknown Error\n");
                publish_sensor_status(choice, DEVICE_ERROR);
                break;
            }
        }
    }
}

void test_float_switch(int choice, int switch_status) {

    const char *TAG = "TEST_FLOAT_SWITCH_";
    printf("\n");
    ESP_LOGI(TAG, "Testing float switch ");
    printf("-------------------------------------------------\n");

    if (choice == FS_BOTTOM && gpio_get_level(FLOAT_SWITCH_BOTTOM_GPIO) == FS_TANK_FULL) {
        if (switch_status == DEVICE_ON) {
            gpio_set_intr_type(FLOAT_SWITCH_BOTTOM_GPIO, GPIO_INTR_NEGEDGE);
            gpio_isr_handler_add(FLOAT_SWITCH_BOTTOM_GPIO, bottom_float_switch_isr_handler, NULL);
            if (gpio_isr_handler_add(FLOAT_SWITCH_BOTTOM_GPIO, bottom_float_switch_isr_handler, NULL) == ESP_OK) {
                publish_float_switch_status(choice, DEVICE_ON);
            }
            else {
                publish_float_switch_status(choice, DEVICE_ERROR);
            }
        }
    }
    else if (choice == FS_TOP && gpio_get_level(FLOAT_SWITCH_TOP_GPIO) == FS_TANK_EMPTY) {
        if (switch_status == DEVICE_ON) {
            gpio_set_intr_type(FLOAT_SWITCH_TOP_GPIO, GPIO_INTR_POSEDGE);
            gpio_isr_handler_add(FLOAT_SWITCH_TOP_GPIO, top_float_switch_isr_handler, NULL);
            if (gpio_isr_handler_add(FLOAT_SWITCH_TOP_GPIO, top_float_switch_isr_handler, NULL) == ESP_OK) {
                publish_float_switch_status(choice, DEVICE_ON);
            }
            else {
                publish_float_switch_status(choice, DEVICE_ERROR);
            }
        }
    }
    else {
        ESP_LOGE(TAG, "Invalid float switch type");
    }
}
