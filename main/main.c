#include "boot.h"
#include "test_hardware.h"

void app_main(void) {	// Main Method
	// Initiate boot
	boot_sequence();

	// Test all Hardware 
	// test_hardware();
}
