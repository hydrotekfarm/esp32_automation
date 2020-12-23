#define INIT_PROPERTIES_KEY "INIT_PROPS"
#define WIFI_SSID_KEY = "WIFI_SSID"
#define WIFI_PW_KEY = "WIFI_PW"
#define DEVICE_ID_KEY  = "DEV_ID"
#define TIME_KEY = "TIME"
#define BROKER_IP_KEY = "B_IP"

// Contains all the boot code for esp32
void boot_sequence();

// Restart esp32
void restart_esp32();
