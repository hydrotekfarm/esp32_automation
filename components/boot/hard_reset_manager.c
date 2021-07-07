#include "hard_reset_manager.h"

#include <time.h>
#include <sys/time.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include <esp32/ulp.h>
#include <driver/gpio.h>
#include <driver/touch_pad.h>
#include <driver/adc.h>
#include <driver/rtc_io.h>
#include <soc/sens_periph.h>
#include <soc/rtc.h>
#include <driver/timer.h>
#include <driver/periph_ctrl.h>
#include "nvs_manager.h"

#include "ports.h"

unsigned long get_current_time() {
    /* Get time */
    if (gettimeofday(&current_time, NULL) == 0) {
        return current_time.tv_sec;
    }
    return 0UL; 
}

static void IRAM_ATTR reset_button_isr_handler (void* arg) {
   unsigned long start = get_current_time();
   //Make sure we get valid start time // 
   while (start == 0) {
       start = get_current_time();
   }
   //Keep checking if button is pressed at lest 10 seconds then perform reset tasks//
   while (gpio_get_level(HARD_RESET_GPIO) == 1) {
        int curr_time = get_current_time();
        if (curr_time != 0 && (curr_time - start >= 10)) {
            ESP_LOGI(HARD_RESET_TAG, "Hard Rest Initiated.");
            nvs_clear();
            esp_restart();
            ESP_LOGI(HARD_RESET_TAG, "Hard Rest Completed.");
            break; 
        }
   }
}

void init_hard_reset_button() {
	// Create Rising Edge Interrupt on Hard Reset Button GPIO
	gpio_config_t gpio_conf;
	gpio_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
	gpio_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
	gpio_conf.mode = GPIO_MODE_INPUT;
	gpio_conf.pull_up_en = 1;
	gpio_config(&gpio_conf);

	// Install GPIO ISR Service
	gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

	// ISR handler for specific GPIO pin
	gpio_isr_handler_add(HARD_RESET_GPIO, reset_button_isr_handler, (void*) HARD_RESET_GPIO);
}
