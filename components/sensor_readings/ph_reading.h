#include <stdbool.h>

// Global ph variable
float _ph = 0;

// PH measuring status
bool ph_active = true;

// Calibration status
bool ph_calibration = false;

// Measures water ph
void measure_ph();
