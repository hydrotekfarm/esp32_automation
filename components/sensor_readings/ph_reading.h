#include <stdbool.h>

// Global ph variable
float _ph;

// PH measuring status
bool ph_active;

// Calibration status
bool ph_calibration;

// Measures water ph
void measure_ph();
