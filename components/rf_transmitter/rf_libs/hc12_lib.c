#include "hc12_lib.h"

#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include <esp_err.h>
#include "ports.h"


#define NOP() asm volatile ("nop")


void uart_config_driver (int16_t transmit_pin, int16_t recieve_pin) {
    const uart_port_t uart_num = UART_NUM_1;

    uart_config_t uart_config = {
    .baud_rate = 9600,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .rx_flow_ctrl_thresh = 0,
    };

    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));

    // Set UART pins(TX: IO10, RX: IO9)
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1,transmit_pin, recieve_pin, 0, 0));

    // Install UART driver using an event queue here
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, UART_BUFFER_SIZE, UART_BUFFER_SIZE, 10, &uart_queue, 0));
}

void configure_protocol(int32_t pulse_width, int32_t repeat_transmission, int16_t transmit_pin, int16_t recieve_pin, struct binary_bits low_bit, struct binary_bits high_bit, struct binary_bits sync_bit){

	power_outlet_protocol.pulse_width = pulse_width;
	power_outlet_protocol.repeat_transmission = repeat_transmission;
	power_outlet_protocol.low_bit = low_bit;
	power_outlet_protocol.high_bit = high_bit;
	power_outlet_protocol.sync_bit = sync_bit;
	power_outlet_protocol.transmit_pin = transmit_pin;
    power_outlet_protocol.recieve_pin = recieve_pin; 

    //Setup UART driver configuration//
    uart_config_driver(power_outlet_protocol.transmit_pin, power_outlet_protocol.recieve_pin);
}

unsigned long IRAM_ATTR micros(){
	return (unsigned long) (esp_timer_get_time());
}

void IRAM_ATTR delayMicroseconds(uint32_t us)
{
    uint32_t m = micros();
    if(us){
        uint32_t e = (m + us);
        if(m > e){ //overflow
            while(micros() > e){
                NOP();
            }
        }
        while(micros() < e){
            NOP();
        }
    }
}

void transmit_message(unsigned long code, int32_t length){
    const char binary_one = 0x01; 
    const char binary_zero = 0x00; 
	// Repeat Transmission for better accuracy
	for(int i = 0; i < power_outlet_protocol.repeat_transmission; i++){
		for(int j = length - 1; j >= 0; j--){
			if(code & (1L << j)){
				// Transmit Binary 1
                uart_write_bytes(UART_NUM_1, &binary_one, sizeof(binary_one)); 
				delayMicroseconds(power_outlet_protocol.pulse_width * power_outlet_protocol.high_bit.high_pulse_amount);
				uart_write_bytes(UART_NUM_1, &binary_zero, sizeof(binary_zero));
				delayMicroseconds(power_outlet_protocol.pulse_width * power_outlet_protocol.high_bit.low_pulse_amount);
			} else {
				// Transmit Binary 0
				uart_write_bytes(UART_NUM_1, &binary_one, sizeof(binary_one)); 
				delayMicroseconds(power_outlet_protocol.pulse_width * power_outlet_protocol.low_bit.high_pulse_amount);
				uart_write_bytes(UART_NUM_1, &binary_zero, sizeof(binary_zero));
				delayMicroseconds(power_outlet_protocol.pulse_width *  power_outlet_protocol.low_bit.low_pulse_amount);
			} 
		}
		uart_write_bytes(UART_NUM_1, &binary_one, sizeof(binary_one)); 
		delayMicroseconds(power_outlet_protocol.pulse_width * power_outlet_protocol.sync_bit.high_pulse_amount);
		uart_write_bytes(UART_NUM_1, &binary_zero, sizeof(binary_zero)); 
		delayMicroseconds(power_outlet_protocol.pulse_width * power_outlet_protocol.sync_bit.low_pulse_amount);
		uart_write_bytes(UART_NUM_1, &binary_zero, sizeof(binary_zero)); 
	}
}

void send_message(const char* rf_address_ptr, const char* power_outlet_state_ptr){
	unsigned long code = 0;
	unsigned int length = 0;

	  for (const char* p = rf_address_ptr; *p; p++) {
		  // convert char to unsigned long
	    code <<= 1L;
	    if (*p != '0')
	      code |= 1L;
	    length++;	// Calculate length of binary
	  }

	  for (const char* p = power_outlet_state_ptr; *p; p++) {
		  // convert char to unsigned long
	    code <<= 1L;
	    if (*p != '0')
	      code |= 1L;
	    length++;	// Calculate length of binary
	  }

	  // Transmit combined message
	  transmit_message(code, length);
	  ESP_LOGI("Transmission", "%lu", code);
}


