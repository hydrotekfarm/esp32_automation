#include <stdbool.h>
#include <stdio.h>

#ifndef NVS_MANAGER_H
#define NVS_MANAGER_H

#define NVS_TAG "NVS_MANAGER"

// Enum storing data types compatible with NVS
enum NVS_DATA_TYPES {
	UINT8,
	INT8,
	UINT16,
	INT16,
	UINT32,
	INT32,
	UINT64,
	INT64,
	FLOAT,
	STRING
};

// Linked list to store data needed to be committed to NVS
struct NVS_Data {
	struct NVS_Data *next;
	char *key;
	enum NVS_DATA_TYPES data_type;
	void *datum;
};


#endif


// ----------------------------------------------------- MEMBER FUNCTIONS -----------------------------------------------------------------------

// Initializes nvs for use
void init_nvs();

// Clear nvs data
void nvs_clear();

// Start inputting new data for commitment. Returns true if currently possible, false otherwise
struct NVS_Data* nvs_init_data();

// Add data to linked list to be stored for commitment
void nvs_add_data(struct NVS_Data *data, char *key_in, enum NVS_DATA_TYPES data_type_in, void *datum_in);

// Commits data entered thus far to
bool nvs_commit_data(struct NVS_Data *data, char *nvs_namespace);

// NVS getters
bool nvs_get_uint8(char *namespace, char *key, uint8_t *data);
bool nvs_get_int8(char *namespace, char *key, int8_t *data);
bool nvs_get_uint16(char *namespace, char *key, uint16_t *data);
bool nvs_get_int16(char *namespace, char *key, int16_t *data);
bool nvs_get_uint32(char *namespace, char *key, uint32_t *data);
bool nvs_get_int32(char *namespace, char *key, int32_t *data);
bool nvs_get_uint64(char *namespace, char *key, uint64_t *data);
bool nvs_get_int64(char *namespace, char *key, int64_t *data);
bool nvs_get_float(char *namespace, char *key, float *data);
bool nvs_get_string(char *namespace, char *key, char *data);
