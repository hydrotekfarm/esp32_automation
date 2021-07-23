#include "led_manager.h"
#include "ports.h"
#include "wifi_connect.h"

void wifi_led(void *args) {
    for (;;) {
        if (get_is_wifi_connected()) {
            gpio_set_level(BLUE_LED, 1);
            vTaskDelay(pdMS_TO_TICKS(5000));
        } else {
            //If not Connected then blink blue led//
            gpio_set_level(BLUE_LED, 1);
            vTaskDelay(pdMS_TO_TICKS(1000));
            gpio_set_level(BLUE_LED, 0);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}