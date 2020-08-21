#include <stdbool.h>

// Global ph variable
static float _ph = 0;

// PH measuring status
static bool ph_active = true;

// Calibration status
static bool ph_calibration = false;

// Measures water ph
void measure_ph();
