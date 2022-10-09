#include <esp_log.h>
#include "ports.h"

void init_ports() {
	// Initialize MCP23017 GPIO Expansion
	memset(&ports_dev, 0, sizeof(mcp23x17_t));
	ESP_ERROR_CHECK(mcp23x17_init_desc(&ports_dev, 0, MCP23X17_ADDR_BASE, SDA_GPIO, SCL_GPIO));

	// Initialize GPIO Expansion Ports
	mcp23x17_set_mode(&ports_dev, PH_UP_PUMP_GPIO, MCP23X17_GPIO_OUTPUT);
	mcp23x17_set_mode(&ports_dev, PH_DOWN_PUMP_GPIO, MCP23X17_GPIO_OUTPUT);
	mcp23x17_set_mode(&ports_dev, EC_NUTRIENT_1_PUMP_GPIO, MCP23X17_GPIO_OUTPUT);
	mcp23x17_set_mode(&ports_dev, EC_NUTRIENT_2_PUMP_GPIO, MCP23X17_GPIO_OUTPUT);
	mcp23x17_set_mode(&ports_dev, EC_NUTRIENT_3_PUMP_GPIO, MCP23X17_GPIO_OUTPUT);
	mcp23x17_set_mode(&ports_dev, EC_NUTRIENT_4_PUMP_GPIO, MCP23X17_GPIO_OUTPUT);
	mcp23x17_set_mode(&ports_dev, EC_NUTRIENT_5_PUMP_GPIO, MCP23X17_GPIO_OUTPUT);
	mcp23x17_set_mode(&ports_dev, EC_NUTRIENT_6_PUMP_GPIO, MCP23X17_GPIO_OUTPUT);
	set_gpio_off(PH_UP_PUMP_GPIO);
	set_gpio_off(PH_DOWN_PUMP_GPIO);
}

esp_err_t set_gpio_on(int gpio) {
	return mcp23x17_set_level(&ports_dev, gpio, 1);
}
esp_err_t set_gpio_off(int gpio) {
	return mcp23x17_set_level(&ports_dev, gpio, 0);	
}



