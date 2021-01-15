#include "control_task.h"

#include <esp_log.h>
#include <sdkconfig.h>

#include "sensor_control.h"
#include "reservoir_control.h"
#include "ph_control.h"
#include "ec_control.h"
#include "sync_sensors.h"
#include "ports.h"
#include "rf_transmitter.h"

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

	char ph_data[] = "{\"ph\":{\"monit_only\":true,\"control\":{\"up_ctrl\":true,\"down_ctrl\":true,\"dose_time\":10,\"dose_interv\":600,\"d/n_enabled\":false,\"day_tgt\":6,\"night_tgt\":6,\"tgt\":6,\"pumps\":{\"pump_1_enbld\":true,\"pump_2_enbld\":false}},\"alarm_min\":3,\"alarm_max\":7}}";
	cJSON *ph_item = cJSON_Parse(ph_data);
	ph_item = ph_item->child;

	char ec_data[] = "{\"ec\":{\"monit_only\":false,\"control\":{\"up_ctrl\":true,\"down_ctrl\":false,\"dose_time\":30,\"dose_interv\":600,\"d/n_enabled\":false,\"day_tgt\":1.5,\"night_tgt\":1.5,\"tgt\":1.5,\"pumps\":{\"pump_1\":{\"enbld\":true,\"value\":10},\"pump_2\":{\"enbld\":false,\"value\":4},\"pump_3\":{\"enbld\":true,\"value\":2},\"pump_4\":{\"enbld\":false,\"value\":7},\"pump_5\":{\"enbld\":true,\"value\":3}}},\"alarm_min\":1.5,\"alarm_max\":4}}";
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
