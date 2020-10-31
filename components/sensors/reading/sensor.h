#include <stdbool.h>
#include <Freertos/freertos.h>
#include <Freertos/task.h>

struct sensor {
	TaskHandle_t task_handle;
	double current_value;
	bool is_active;
	bool is_calib;
};
