// Karl Palsson, 2010
// 
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h> 
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/interrupt.h>
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

#define ADC_ENABLE  (ADCSRA |= (1<<ADEN))
#define ADC_DISABLE  (ADCSRA &= ~(1<<ADEN))

#define BAUD_RATE 19200


#define VREF_VCC  0
#define VREF_11  (1<<REFS1) | (1<<REFS0) // requires external cap at aref!

void init_adc(void) {
    ADMUX = VREF_11 | 0;  // adc0
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
        XBEE_OFF;
        clock_prescale_set(0);
        ADCSRA = (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);  // enable ADC
	uart_init(UART_BAUD_SELECT(BAUD_RATE,F_CPU));
}


int main(void) {
	init();
	sei();
        kpacket packet;
        packet.header = 'x';

	LED_ON;
	while (1) {
                ADC_ENABLE;
                init_adc();
		unsigned int sensor1 = adc_read();
                ADC_DISABLE;
                
		LED_ON;
		unsigned int freq1 = readSensorFreq();
		LED_OFF;

                packet.type1 = 37;
                packet.value1 = sensor1;
                packet.type2 = 'f';
                packet.value2 = freq1;

		XBEE_ON;
		_delay_ms(15); // xbee manual says 2ms for sleep mode 2, 13 for sm1
                xbee_send_16(1, packet);
		XBEE_OFF;
		_delay_ms(2000);
	}
}

