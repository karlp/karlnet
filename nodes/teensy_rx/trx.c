// Karl Palsson, 2010
// 
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h> 
#include <avr/pgmspace.h>
#include <util/delay.h>
// usb hid...
#include "usb_debug_only.h"
#include "print.h"
// pjrc uart!
#include "pjrc_uart.h"

// from teensy code samples...
#define LED_ON          (PORTD |= (1<<6))
#define LED_OFF         (PORTD &= ~(1<<6))
#define LED_CONFIG      (DDRD |= (1<<6))
#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))

#define BAUD_RATE 19200



void init(void) {
	LED_CONFIG;

	usb_init();
	
	uart_init(BAUD_RATE);

	while (!usb_configured()) {
		LED_ON;
		_delay_ms(90);
		LED_OFF;
		_delay_ms(90);
	}
	// Advised to wait a bit longer by pjrc.com
	// as not all drivers will be ready even after host is up
	_delay_ms(500);
}


int main(void) {
	CPU_PRESCALE(0);
	init();

	LED_ON;
	print("woke up...woo\n");
	while (1) {
                int i = 0;
                while (uart_available()) {
                    LED_ON;
                    uint8_t rx = uart_getchar();
                    i++;
                    phex(rx);
                }
		LED_OFF;
	}
}

