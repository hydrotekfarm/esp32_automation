#include <stdbool.h>

#ifndef NVS_MANAGER_H
#define NVS_MANAGER_H

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
	STRING
};

// Linked list to store data needed to be committed to NVS
struct Data {
	struct Data *next;
	char *key;
	enum NVS_DATA_TYPES data_type;
	void *data;
};


#endif

// ----------------------------------------------------- MEMBER VARIABLES -----------------------------------------------------------------------



// ----------------------------------------------------- MEMBER FUNCTIONS -----------------------------------------------------------------------

// Initializes nvs for use
void init_nvs();

// Start inputting new data for commitment. Returns true if currently possible, false otherwise
void init_data(struct Data *data);

// Add data to linked list to be stored for commitment
void add_data(struct Data *data, char *key, enum NVS_DATA_TYPES data_type, void *datum);

// Commits data entered thus far to
bool commit_data(char *nvs_namespace);

// Gets data stored in nvs and returns as void*
void* get_data(char *nvs_namespace, char *key, enum NVS_DATA_TYPES data_type);
