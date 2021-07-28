#include "test_hardware.h"
#include "ports.h"
#include "rf_lib.h"
#include "rf_transmitter.h"

void test_hardware() {
    gpio_set_level(GREEN_LED, 1);

    // Init i2cdev
	ESP_ERROR_CHECK(i2cdev_init());
    init_ports();
    init_rf_protocol();

    test_mcp23017();
    test_rf();
}

void test_mcp23017() {
    set_gpio_on(PH_DOWN_PUMP_GPIO);
    set_gpio_on(PH_UP_PUMP_GPIO);
    set_gpio_on(EC_NUTRIENT_1_PUMP_GPIO);
    set_gpio_on(EC_NUTRIENT_2_PUMP_GPIO);
    set_gpio_on(EC_NUTRIENT_3_PUMP_GPIO);
    set_gpio_on(EC_NUTRIENT_4_PUMP_GPIO);
    set_gpio_on(EC_NUTRIENT_5_PUMP_GPIO);

    vTaskDelay(pdMS_TO_TICKS(5000));

    set_gpio_off(PH_DOWN_PUMP_GPIO);
    set_gpio_off(PH_UP_PUMP_GPIO);
    set_gpio_off(EC_NUTRIENT_1_PUMP_GPIO);
    set_gpio_off(EC_NUTRIENT_2_PUMP_GPIO);
    set_gpio_off(EC_NUTRIENT_3_PUMP_GPIO);
    set_gpio_off(EC_NUTRIENT_4_PUMP_GPIO);
    set_gpio_off(EC_NUTRIENT_5_PUMP_GPIO);
}

void test_rf() {
    // address: any 20 digit binary, state (on/off): (0011, 1100)
    for(int i = 0; i < 10; i++) {
        send_message("00010100010101010011", "0011");
        vTaskDelay(pdMS_TO_TICKS(1000));
        send_message("00010100010101010011", "1100");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}