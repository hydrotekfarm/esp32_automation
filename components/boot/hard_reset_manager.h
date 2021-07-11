#define ESP_INTR_FLAG_DEFAULT 0
#define GPIO_INPUT_PIN_SEL_RESET (1ULL << HARD_RESET_GPIO);

#define HARD_RESET_TAG "HARD_RESET_MANAGER"

struct timeval current_time; 

void init_hard_reset_button();
