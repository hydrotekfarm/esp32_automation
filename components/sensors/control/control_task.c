#include "control_task.h"
#include "sensor_control.h"
#include "reservoir_control.h"
#include "ph_control.h"
#include "ec_control.h"
#include "sync_sensors.h"
#include "ports.h"
#include <cjson.h> //TODO remove later
#include "rf_transmitter.h"
#include "FreeRTOS/queue.h"
#include <esp_log.h>

void sensor_control (void *parameter) {

	// Float Switch Port Setup
	gpio_pad_select_gpio(FLOAT_SWITCH_TOP_GPIO);
	gpio_set_direction(FLOAT_SWITCH_TOP_GPIO, GPIO_MODE_INPUT);

	gpio_pad_select_gpio(FLOAT_SWITCH_BOTTOM_GPIO);
	gpio_set_direction(FLOAT_SWITCH_BOTTOM_GPIO, GPIO_MODE_INPUT);

	ec_nutrient_proportions[0] = 0.40;
	ec_nutrient_proportions[1] = 0.40;
	ec_nutrient_proportions[2] = 0.20;
	ec_nutrient_proportions[3] = 0;
	ec_nutrient_proportions[4] = 0;
	ec_nutrient_proportions[5] = 0;

	ec_pump_gpios[0] = EC_NUTRIENT_1_PUMP_GPIO;
	ec_pump_gpios[1] = EC_NUTRIENT_2_PUMP_GPIO;
	ec_pump_gpios[2] = EC_NUTRIENT_3_PUMP_GPIO;
	ec_pump_gpios[3] = EC_NUTRIENT_4_PUMP_GPIO;
	ec_pump_gpios[4] = EC_NUTRIENT_5_PUMP_GPIO;
	ec_pump_gpios[5] = EC_NUTRIENT_6_PUMP_GPIO;

	char ph_data[] = "{\"ph\":{\"monitoring_only\":true,\"control\":{\"up_control\":true,\"down_control\":true,\"dosing_time\":10,\"dosing_interval\":600,\"day_and_night\":false,\"day_target_value\":6,\"night_target_value\":6,\"target_value\":6,\"pumps\":{\"pump_1_enabled\":true,\"pump_2_enabled\":false}},\"alarm_min\":3,\"alarm_max\":7}}";
	cJSON *ph_item = cJSON_Parse(ph_data);
	ph_item = ph_item->child;

	char ec_data[] = "{\"ec\":{\"monitoring_only\":false,\"control\":{\"up_control\":true,\"down_control\":false,\"dosing_time\":30,\"dosing_interval\":600,\"day_and_night\":false,\"day_target_value\":1.5,\"night_target_value\":1.5,\"target_value\":1.5,\"pumps\":{\"pump_1\":{\"enabled\":true,\"value\":10},\"pump_2\":{\"enabled\":false,\"value\":4},\"pump_3\":{\"enabled\":true,\"value\":2},\"pump_4\":{\"enabled\":false,\"value\":7},\"pump_5\":{\"enabled\":true,\"value\":3}}},\"alarm_min\":1.5,\"alarm_max\":4}}";
	cJSON *ec_item = cJSON_Parse(ec_data);
	ec_item = ec_item->child;

	init_sensor_control(get_ph_control(), "PH_CONTROL", ph_item, PH_MARGIN_ERROR);
	init_doser_control(get_ph_control());

	init_sensor_control(get_ec_control(), "EC_CONTROL", ec_item, EC_MARGIN_ERROR);
	init_doser_control(get_ec_control());

	water_in_rf_message.rf_address_ptr = water_in_address;
	water_out_rf_message.rf_address_ptr = water_out_address;

	for(;;)  {
		// Check sensors
		if(reservoir_control_active) check_water_level(); // TODO remove if statement for consistency
		check_ph();
		check_ec();

		// Wait till next sensor readings
		vTaskDelay(pdMS_TO_TICKS(SENSOR_MEASUREMENT_PERIOD));
	}
}
