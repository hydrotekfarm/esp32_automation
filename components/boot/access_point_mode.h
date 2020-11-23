#include <freertos/event_groups.h>
#include <esp_http_server.h>

#define EXAMPLE_ESP_WIFI_SSID      "Hydrotek Dosing System"      //CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      "hydrotek"   //CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_WIFI_CHANNEL   1            //CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN       1            //CONFIG_ESP_MAX_STA_CONN

// WiFi Coordination with Event Group
EventGroupHandle_t json_information_event_group;

// JSON Information event group bit
#define INFORMATION_TRANSFERRED_BIT (1<<0)

void start_access_point_mode();

void stop_webserver(httpd_handle_t server);

httpd_handle_t start_webserver(void);


