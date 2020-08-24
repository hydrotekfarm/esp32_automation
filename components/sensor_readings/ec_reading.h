#include <stdbool.h>

// Global ec variable
float _ec = 0;

// EC measuring status
bool ec_active = true;

// Calibration status
bool ec_calibration = false;

// Measures water ph
void measure_ec();
