#include <stdbool.h>

// Global water temp variable
float _water_temp = 0;

// Water temperature measuring status
bool water_temperature_active = false;

// Measures water temperature
void measure_water_temperature();
