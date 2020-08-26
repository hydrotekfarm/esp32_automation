#include "rf_lib.h"

struct binary_bits low_bit;
struct binary_bits sync_bit;
struct binary_bits high_bit;

// Initialize rf bits
void init_rf();

// Send rf message
void send_rf_transmission();
