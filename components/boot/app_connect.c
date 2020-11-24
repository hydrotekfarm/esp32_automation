#include "app_connect.h"

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

static const char *TAG = "WIFI_AP_HTTP_SERVER";

// HTTP POST handler
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

void init_app_connect() {
	bool has_properties = true; // TODO check if ssid, pw, and broker ip are stored in NVS
	if(has_properties) return;

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
	vTaskDelay(pdMS_TO_TICKS(1000));
}

