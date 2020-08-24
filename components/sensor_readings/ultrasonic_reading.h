#include <stdbool.h>
#include <inttypes.h>

// Max measuring distance
#define MAX_DISTANCE_CM 500

// Global distance variable
float _distance;

// Distance measuring status
bool ultrasonic_active;

// Measures water level distance
void measure_distance();
