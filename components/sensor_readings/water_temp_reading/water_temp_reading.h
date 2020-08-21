#include <stdbool.h>

// Global water temp variable
static float _water_temp = 0;

// Water temperature measuring status
static bool water_temperature_active = false;

// Measures water temperature
void measure_water_temperature();
