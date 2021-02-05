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

void push_grow_status() {
	// Store in NVS
	struct NVS_Data *data = nvs_init_data();
	nvs_add_data(data, GROW_ACTIVE_KEY, UINT8, &is_grow_active);
}

void push_grow_settings_status() {
	// Store in NVS
	struct NVS_Data *data = nvs_init_data();
	nvs_add_data(data, GROW_ACTIVE_KEY, UINT8, &is_settings_received);
}

void start_grow_cycle() {
	// Don't start unless settings have been received
	if(!is_settings_received) return;

	// Set active to true and store in NVS
	is_grow_active = true;
	push_grow_status();
}

void stop_grow_cycle() {
	// Set active to false ands store in NVS
	is_grow_active = false;
	push_grow_status();
}

void settings_received() {
	push_grow_settings_status();
	is_settings_received = true;
}

bool get_is_settings_received() { return is_settings_received; }
bool get_is_grow_active() { return is_grow_active; }
