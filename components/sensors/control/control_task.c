#include "control_task.h"

#include <esp_log.h>
#include <sdkconfig.h>

#include "sensor_control.h"
#include "reservoir_control.h"
#include "ph_control.h"
#include "ec_control.h"
#include "water_temp_control.h"
#include "control_settings_keys.h"
#include "sync_sensors.h"
#include "ports.h"
#include "mqtt_manager.h"
#include "rf_transmitter.h"

void init_control() {
	ec_pump_gpios[0] = EC_NUTRIENT_1_PUMP_GPIO;
	ec_pump_gpios[1] = EC_NUTRIENT_2_PUMP_GPIO;
	ec_pump_gpios[2] = EC_NUTRIENT_3_PUMP_GPIO;
	ec_pump_gpios[3] = EC_NUTRIENT_4_PUMP_GPIO;
	ec_pump_gpios[4] = EC_NUTRIENT_5_PUMP_GPIO;
	ec_pump_gpios[5] = EC_NUTRIENT_6_PUMP_GPIO;

	// Float Switch Port Setup
	gpio_pad_select_gpio(FLOAT_SWITCH_TOP_GPIO);
	gpio_set_direction(FLOAT_SWITCH_TOP_GPIO, GPIO_MODE_INPUT);

	gpio_pad_select_gpio(FLOAT_SWITCH_BOTTOM_GPIO);
	gpio_set_direction(FLOAT_SWITCH_BOTTOM_GPIO, GPIO_MODE_INPUT);

	init_sensor_control(get_ph_control(), "PH_CONTROL", get_ph_control_status(), PH_MARGIN_ERROR);
	init_doser_control(get_ph_control());

	init_sensor_control(get_ec_control(), "EC_CONTROL", get_ec_control_status(), EC_MARGIN_ERROR);
	init_doser_control(get_ec_control());

	init_sensor_control(get_water_temp_control(), "WATER_TEMP_CONTROL", get_water_temp_control_status(), WATER_TEMP_MARGIN_ERROR);
	is_water_cooler_on = false;

	init_reservoir();

	water_in_rf_message.rf_address_ptr = water_in_address;
	water_out_rf_message.rf_address_ptr = water_out_address;
}

void sensor_control (void *parameter) {
	for(;;)  {
		// Check sensors
		if(reservoir_control_active) check_water_level(); // TODO remove if statement for consistency
		check_ph();
		check_ec();
		check_water_temp();

		// Wait till next sensor readings
		vTaskDelay(pdMS_TO_TICKS(SENSOR_MEASUREMENT_PERIOD));
	}
}
