#include <stdbool.h>

// Global ec variable
static float _ec = 0;

// EC measuring status
static bool ec_active = true;

// Calibration status
static bool ec_calibration = false;

// Measures water ph
void measure_ph();
