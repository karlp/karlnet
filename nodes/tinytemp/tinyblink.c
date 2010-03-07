// Karl Palsson, 2010
// 
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h> 
#include <avr/pgmspace.h>
#include <util/delay.h>

// from teensy code samples...
#define LED_ON          (PORTB |= (1<<PB4))
#define LED_OFF         (PORTB &= ~(1<<PB4))
#define LED_CONFIG      (DDRB |= (1<<PB4))
#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))


void init(void) {
	LED_CONFIG;
	CPU_PRESCALE(0);
}


int main(void) {
	init();
	LED_ON;
	while (1) {
		LED_ON;
		_delay_ms(300);
		LED_OFF;
		_delay_ms(300);
	}
}

