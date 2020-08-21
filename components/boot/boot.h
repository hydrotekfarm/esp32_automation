#include <freertos/event_groups.h>

char *TAG = "BOOT";

// WiFi Coordination with Event Group
static EventGroupHandle_t wifi_event_group;

// WiFi bits
#define WIFI_CONNECTED_BIT (1<<0)
#define WIFI_FAIL_BIT      (1<<1)

#define RETRYMAX 5 // WiFi MAX Reconnection Attempts

static int retryNumber = 0;  // WiFi Reconnection Attempts

// Contains all the boot code for esp32
void boot_sequence();
