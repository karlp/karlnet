// Karl Palsson, 2010
// 
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h> 
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "uart.h"
#include "karlnet.h"
#include "xbee-api.h"

#include "FreqCounter.h"

#define LED_ON          (PORTD |= (1<<PIND2))
#define LED_OFF         (PORTD &= ~(1<<PIND2))
#define LED_CONFIG      (DDRD |= (1<<PIND2))

#define XBEE_OFF        (PORTD |= (1<<PIND3))
#define XBEE_ON         (PORTD &= ~(1<<PIND3))
#define XBEE_CONFIG     (DDRD |= (1<<PIND3))

#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))

#define BAUD_RATE 19200

#define VREF_VCC  0
#define VREF_11  (1<<REFS1) | (1<<REFS0) // requires external cap at aref!


void adc_config(void)
{
    ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);  // enable ADC
    ADMUX = VREF_11;  // vref and adc0
}


unsigned int adc_read(void)
{
    ADCSRA |= (1<<ADSC);               // begin the conversion
    while (ADCSRA & (1<<ADSC)) ;   // wait for the conversion to complete
    unsigned char lsb = ADCL;                              // read the LSB first
    return (ADCH << 8) | lsb;          // read the MSB and return 10 bit result
}

void uart_print_short(unsigned int val) {
        uart_putc((unsigned char)(val >> 8));
        uart_putc((unsigned char)(val & 0xFF));
}

void uart_print_header(void) {
        uart_putc('x');
        uart_putc('x');
        uart_putc('x');
}

unsigned int readSensorFreq(void) {
	// this gives you an answer in hz, but only up to 32k :)
	return blockingRead(1, 1000);
}


void init(void) {
	LED_CONFIG;
	XBEE_CONFIG;
	adc_config();
	uart_init(UART_BAUD_SELECT(BAUD_RATE,F_CPU));
}


int main(void) {
	CPU_PRESCALE(0);
	init();
	sei();

	LED_ON;
	while (1) {
		unsigned int sensor1 = adc_read();
		LED_ON;
		unsigned int freq1 = readSensorFreq();
		LED_OFF;
		XBEE_ON;
		_delay_ms(20);
		uart_puts_P("xxx");
		uart_putc(37);
		uart_print_short(sensor1);
		uart_putc('f');
		uart_print_short(freq1);
		XBEE_OFF;
		_delay_ms(2000);
	}
}

