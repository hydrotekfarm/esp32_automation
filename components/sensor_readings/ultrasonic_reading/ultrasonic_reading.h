#include <stdbool.h>
#include <inttypes.h>

// Global distance variable
static float _distance = 0;

// Distance measuring status
static bool ultrasonic_active = false;

// Max measuring distance
static uint32_t max_distance_cm = 500;

// Measures water level distance
void measure_distance();
