#include <freertos/event_groups.h>

// WiFi Coordination with Event Group
static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT (1<<0)
#define WIFI_FAIL_BIT      (1<<1)


void boot_sequence();
