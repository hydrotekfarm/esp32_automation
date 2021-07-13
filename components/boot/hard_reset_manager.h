#include <Freertos/freertos.h>
#include <Freertos/task.h>
#include <Freertos/semphr.h>
#define ESP_INTR_FLAG_DEFAULT 0
#define GPIO_INPUT_PIN_SEL_RESET (1ULL << HARD_RESET_GPIO);

#define HARD_RESET_TAG "HARD_RESET_MANAGER"

TaskHandle_t hard_reset_task_handle;
SemaphoreHandle_t xBinarySemph;

struct timeval current_time; 

esp_err_t init_reset_semaphore();

void hard_reset_task(void *args);

void init_hard_reset_button();
