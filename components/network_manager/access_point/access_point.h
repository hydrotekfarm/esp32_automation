#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_http_server.h>

#define EXAMPLE_ESP_WIFI_SSID      "Hydrotek Dosing System"      //CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      "hydrotek"   //CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_WIFI_CHANNEL   1            //CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN       1            //CONFIG_ESP_MAX_STA_CONN

#define INIT_PROPERTIES_KEY "INIT_PROPS"
#define WIFI_SSID_KEY = "WIFI_SSID"
#define WIFI_PW_KEY = "WIFI_PW"
#define DEVICE_ID_KEY  = "DEV_ID"
#define TIME_KEY = "TIME"
#define BROKER_IP_KEY = "B_IP"

// JSON Information event group bit
#define INFORMATION_TRANSFERRED_BIT (1<<0)

// WiFi Coordination with Event Group
EventGroupHandle_t json_information_event_group;

// Set wifi ssid and pw, broker ip, device id, and time
void init_network_properties();

// Start access point mode to receive network properties
void init_access_point_mode();


