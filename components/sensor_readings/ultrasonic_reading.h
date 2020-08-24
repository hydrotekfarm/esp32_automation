#include <stdbool.h>
#include <inttypes.h>

// Global distance variable
float _distance = 0;

// Distance measuring status
bool ultrasonic_active = false;

// Max measuring distance
uint32_t max_distance_cm = 500;

// Measures water level distance
void measure_distance();
