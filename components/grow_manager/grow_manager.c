#include "grow_manager.h"

#include <esp_system.h>

#include "nvs_manager.h"
#include "nvs_namespace_keys.h"

void init_grow_manager() {
	uint8_t status; // Holds vars coming from NVS

	// Check for sensor settings status
	if(!nvs_get_data(&status, GROW_SETTINGS_NVS_NAMESPACE, SETTINGS_RECEIVED_KEY, UINT8) || !status) {
		is_settings_received = false;
	} else {
		is_settings_received = true;
	}

	// Check for grow cycle status
	if(!nvs_get_data(&status, GROW_SETTINGS_NVS_NAMESPACE, GROW_ACTIVE_KEY, UINT8) || !status) {
		is_grow_active = false;
	} else {
		is_grow_active = true;
	}
}

void start_grow_cycle() {
	is_grow_active = true;

	// TODO start grow cycle
}

void stop_grow_cycle() {
	is_grow_active = false;

	// TODO stop grow cycle
}

void settings_received() {
	is_settings_received = true;
}

bool get_is_settings_received() { return is_settings_received; }
bool get_is_grow_active() { return is_grow_active; }
