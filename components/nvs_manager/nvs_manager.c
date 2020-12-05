#include "nvs_manager.h"

#include <stdio.h>
#include <stdlib.h>
#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <nvs.h>

void init_nvs() {
	// TODO implement (copy over from boot)
}

struct Data* nvs_init_data() {
	struct Data *data = malloc(sizeof(struct Data));
	data->next = NULL;
	return data;
}

void nvs_add_data(struct Data *data, char *key_in, enum NVS_DATA_TYPES data_type_in, void *datum_in) {
	struct Data* new_data = nvs_init_data();
	new_data->key = key_in;
	new_data->data_type = data_type_in;
	new_data->datum = datum_in;

	while(data->next != NULL) {
		data = data->next;
	}
	data->next = data;
}

bool nvs_commit_data(struct Data *data, char *nvs_namespace) {
	char *TAG = "NVS_COMMIT_DATA";

	nvs_handle_t handle;
	esp_err_t err  = nvs_open(nvs_namespace, NVS_READWRITE, &handle);
	if(err != ESP_OK) {
		ESP_LOGI(TAG, "Unable to open NVS");
		return false;
	}

	while(data != NULL) {
		switch(data->data_type) {
		case UINT8:
			err = nvs_set_u8(handle, data->key, *(uint8_t*)(data->datum));
			break;
		case INT8:
			err = nvs_set_i8(handle, data->key, *(int8_t*)(data->datum));
			break;
		case UINT16:
			err = nvs_set_u16(handle, data->key, *(uint16_t*)(data->datum));
			break;
		case INT16:
			err = nvs_set_i16(handle, data->key, *(int16_t*)(data->datum));
			break;
		case UINT32:
			err = nvs_set_u32(handle, data->key, *(uint32_t*)(data->datum));
			break;
		case INT32:
			err = nvs_set_i32(handle, data->key, *(int32_t*)(data->datum));
			break;
		case UINT64:
			err = nvs_set_u64(handle, data->key, *(uint64_t*)(data->datum));
			break;
		case INT64:
			err = nvs_set_i64(handle, data->key, *(int64_t*)(data->datum));
			break;
		case FLOAT:
			// TODO add
		case STRING:
			// TODO add
			break;
		}

		if(err != ESP_OK) {
			ESP_LOGI(TAG, "Failed adding data to NVS");
			return false;
		}
		struct Data *temp = data;
		data = data->next;
		free(temp);
	}

	err = nvs_commit(handle);
	if(err != ESP_OK) {
		ESP_LOGI(TAG, "Failed committing data to NVS");
		return false;
	}

	nvs_close(handle);
	return true;
}
