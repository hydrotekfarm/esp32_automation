#include <stdbool.h>

// Global ec variable
float _ec;

// EC measuring status
bool ec_active;

// Calibration status
bool ec_calibration;

// Measures water ph
void measure_ec();
