void test_hardware();

void init_ph();
void init_ec();
void init_water_temperature();

void test_ph();
void test_ec();
void test_mcp23017();
void test_rf();
void test_water_temperature(int water_temperature_status);
void test_motor(int motor_choice, int motor_status);
void test_lights(int light_choice, int light_status);
void test_water_cooler(int water_cooler_status);
void test_water_heater(int water_heater_status);
void test_water_in(int water_in_status);
void test_water_out(int water_out_status);
void test_irrigation(int irrigation_status);
void test_float_switch(int float_switch_type, int float_switch_status);

#define DEVICE_ON 1
#define DEVICE_OFF 0
#define DEVICE_ERROR -1
//Float Switch types
#define FS_BOTTOM 0
#define FS_TOP 1
#define FS_TANK_FULL 1
#define FS_TANK_EMPTY 0