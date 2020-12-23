#include "access_point.h"

#include <esp_http_server.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>
#include "nvs_flash.h"
#include "esp_eth.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "cJSON.h"
#include "nvs_manager.h"
#include "nvs_namespace_keys.h"

static const char *TAG = "WIFI_AP_HTTP_SERVER";

static void json_parser(const char *buffer)
{
   const cJSON *ssid;
   const cJSON *password;
   const cJSON *device_id;
   const cJSON *time;
   const cJSON *broker_ip;

   cJSON *root = cJSON_Parse(buffer);

   if (root == NULL) {
      ESP_LOGI(TAG, "Fail to deserialize Json");
      return;
   }

   ssid = cJSON_GetObjectItemCaseSensitive(root, "ssid");
   if (cJSON_IsString(ssid) && (ssid->valuestring != NULL))
   {
      ESP_LOGI(TAG, "ssid: \"%s\"\n", ssid->valuestring);
   }
   password = cJSON_GetObjectItemCaseSensitive(root, "password");
   if (cJSON_IsString(password) && (password->valuestring != NULL))
   {
      ESP_LOGI(TAG, "password: \"%s\"\n", password->valuestring);
   }
   device_id = cJSON_GetObjectItemCaseSensitive(root, "device_id");
   if (cJSON_IsString(device_id) && (device_id->valuestring != NULL))
   {
      ESP_LOGI(TAG, "device_id: \"%s\"\n", device_id->valuestring);
   }
   time = cJSON_GetObjectItemCaseSensitive(root, "time");
   if (cJSON_IsString(time) && (time->valuestring != NULL))
   {
      ESP_LOGI(TAG, "time: \"%s\"\n", time->valuestring);
   }
   broker_ip = cJSON_GetObjectItemCaseSensitive(root, "broker_ip");
   if (cJSON_IsString(broker_ip) && (broker_ip->valuestring != NULL))
   {
      ESP_LOGI(TAG, "broker_ip: \"%s\"\n", broker_ip->valuestring);
   }
}

/* An HTTP POST handler */
static esp_err_t echo_post_handler(httpd_req_t *req)
{
   char buf[200];
   int ret, remaining = req->content_len;

   while (remaining > 0) {
      /* Read the data for the request */
      if ((ret = httpd_req_recv(req, buf,
                  MIN(remaining, sizeof(buf)))) <= 0) {
         if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            /* Retry receiving if timeout occurred */
            continue;
         }
         return ESP_FAIL;
      }

      remaining -= ret;

      /* Log data received */
      ESP_LOGI(TAG, "=========== RECEIVED DATA ==========");
      ESP_LOGI(TAG, "%.*s", ret, buf);
      ESP_LOGI(TAG, "====================================");
      json_parser(buf);
   }

   // End response
   httpd_resp_send_chunk(req, NULL, 0);
   xEventGroupSetBits(json_information_event_group, INFORMATION_TRANSFERRED_BIT);
   return ESP_OK;
}


static const httpd_uri_t echo = {
   .uri       = "/echo",
   .method    = HTTP_POST,
   .handler   = echo_post_handler,
   .user_ctx  = NULL
};

httpd_handle_t start_webserver(void)
{
   httpd_handle_t server = NULL;
   httpd_config_t config = HTTPD_DEFAULT_CONFIG();

   // Start the httpd server
   ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
   if (httpd_start(&server, &config) == ESP_OK) {
      // Set URI handlers
      ESP_LOGI(TAG, "Registering URI handlers");
      httpd_register_uri_handler(server, &echo);
      return server;
   }

   ESP_LOGI(TAG, "Error starting server!");
   return NULL;
}

void stop_webserver(httpd_handle_t server)
{
   httpd_stop(server); // Stop the httpd server
}

void start_access_point_mode() {
   // WIFI configuration
   wifi_config_t wifi_config = {
	  .ap = {
		 .ssid = EXAMPLE_ESP_WIFI_SSID,
		 .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
		 .channel = EXAMPLE_ESP_WIFI_CHANNEL,
		 .password = EXAMPLE_ESP_WIFI_PASS,
		 .max_connection = EXAMPLE_MAX_STA_CONN,
		 .authmode = WIFI_AUTH_WPA_WPA2_PSK
	  },
   };

   ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
   ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
   ESP_ERROR_CHECK(esp_wifi_start());

   ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
		   EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}

void init_network_properties() {
	ESP_LOGI(TAG, "Checking for init properties");
		// Check if initial properties haven't been initialized before
		uint8_t init_properties_status;
		if(!nvs_get_data(&init_properties_status, SYSTEM_SETTINGS_NVS_NAMESPACE, INIT_PROPERTIES_KEY, UINT8) || init_properties_status == 0) {
			ESP_LOGI(TAG, "Properties not initialized. Starting access point");

			// Creates access point for mobile connection to receive wifi SSID and pw, broker IP address, and station name
			init_access_point_mode();

			ESP_LOGI(TAG, "Access point done. Updating NVS value");
			init_properties_status = 1;
			struct NVS_Data *data = nvs_init_data();
			nvs_add_data(data, INIT_PROPERTIES_KEY, UINT8, &init_properties_status);

			// TODO add rest of data to be pushed to nvs

			nvs_commit_data(data, SYSTEM_SETTINGS_NVS_NAMESPACE);

			ESP_LOGI(TAG, "NVS value updated");
		}

		ESP_LOGI(TAG, "Init properties done");
}

void init_access_point_mode() {
	json_information_event_group = xEventGroupCreate();
	const wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	static httpd_handle_t server = NULL;
	start_access_point_mode();
	server = start_webserver();
	xEventGroupWaitBits(json_information_event_group, INFORMATION_TRANSFERRED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
	stop_webserver(server);
	esp_wifi_disconnect();
	esp_wifi_stop();
	esp_wifi_deinit();
	ESP_LOGI(TAG, "Starting delay");
	vTaskDelay(pdMS_TO_TICKS(1000));
}

