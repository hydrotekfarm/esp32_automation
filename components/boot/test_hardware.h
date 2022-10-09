void test_hardware();

void init_ph();
void init_ec();
void init_water_temperature();

void test_mcp23017();
void test_rf();
void test_motor(int choice, int switch_status);
void test_outlet(int choice, int switch_status);
void test_sensor(char choice[], int switch_status);
void test_float_switch(int choice, int switch_status);

#define DEVICE_ON 1
#define DEVICE_OFF 0
#define DEVICE_ERROR -1
// Float Switch types
#define FS_BOTTOM 0
#define FS_TOP 1
#define FS_TANK_FULL 1
#define FS_TANK_EMPTY 0