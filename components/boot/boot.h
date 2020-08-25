#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

// WiFi bits
#define WIFI_CONNECTED_BIT (1<<0)
#define WIFI_FAIL_BIT      (1<<1)

#define RETRYMAX 5 // WiFi MAX Reconnection Attempts

#define DEFAULT_VREF 1100  // ADC Voltage Reference

// WiFi Coordination with Event Group
EventGroupHandle_t wifi_event_group;

int retryNumber;  // WiFi Reconnection Attempts

// Contains all the boot code for esp32
void boot_sequence();

// Restart esp32
void restart_esp32();
