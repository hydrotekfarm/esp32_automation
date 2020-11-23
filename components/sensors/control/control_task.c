#include "control_task.h"

#include "sensor_control.h"
#include "ph_control.h"
#include "ec_control.h"
#include "sync_sensors.h"
#include "ports.h"
#include <cjson.h> //TODO remove later

void sensor_control (void *parameter) {
	ec_nutrient_proportions[0] = 0.10;
	ec_nutrient_proportions[1] = 0.20;
	ec_nutrient_proportions[2] = 0.30;
	ec_nutrient_proportions[3] = 0.10;
	ec_nutrient_proportions[4] = 0.00;
	ec_nutrient_proportions[5] = 0.30;

	ec_pump_gpios[0] = EC_NUTRIENT_1_PUMP_GPIO;
	ec_pump_gpios[1] = EC_NUTRIENT_2_PUMP_GPIO;
	ec_pump_gpios[2] = EC_NUTRIENT_3_PUMP_GPIO;
	ec_pump_gpios[3] = EC_NUTRIENT_4_PUMP_GPIO;
	ec_pump_gpios[4] = EC_NUTRIENT_5_PUMP_GPIO;
	ec_pump_gpios[5] = EC_NUTRIENT_6_PUMP_GPIO;

	char ph_data[] = "{\"ph\":{\"monitoring_only\":false,\"control\":{\"up_control\":true,\"down_control\":true,\"dosing_time\":10,\"dosing_interval\":600,\"day_and_night\":false,\"day_target_value\":6,\"night_target_value\":6,\"target_value\":6,\"pumps\":{\"pump_1_enabled\":true,\"pump_2_enabled\":false}},\"alarm_min\":3,\"alarm_max\":7}}";
	cJSON *ph_item = cJSON_Parse(ph_data);
	ph_item = ph_item->child;

	char ec_data[] = "{\"ec\":{\"monitoring_only\":false,\"control\":{\"up_control\":true,\"down_control\":true,\"dosing_time\":30,\"dosing_interval\":600,\"day_and_night\":false,\"day_target_value\":23,\"night_target_value\":4,\"target_value\":4,\"pumps\":{\"pump_1\":{\"enabled\":true,\"value\":10},\"pump_2\":{\"enabled\":false,\"value\":4},\"pump_3\":{\"enabled\":true,\"value\":2},\"pump_4\":{\"enabled\":false,\"value\":7},\"pump_5\":{\"enabled\":true,\"value\":3}}},\"alarm_min\":1.5,\"alarm_max\":4}}";
	cJSON *ec_item = cJSON_Parse(ec_data);
	ec_item = ec_item->child;

	init_sensor_control(get_ph_control(), "PH_CONTROL", ph_item, PH_MARGIN_ERROR);
	init_doser_control(get_ph_control());

	init_sensor_control(get_ec_control(), "EC_CONTROL", ec_item, EC_MARGIN_ERROR);
	init_doser_control(get_ec_control());

	for(;;)  {
		// Check sensors
		check_ph();
		check_ec();

		// Wait till next sensor readings
		vTaskDelay(pdMS_TO_TICKS(SENSOR_MEASUREMENT_PERIOD));
	}
}
