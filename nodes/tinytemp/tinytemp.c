// Karl Palsson, 2010
// 
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h> 
#include <util/delay.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include "suart.h"
#include "karlnet.h"
#include "xbee-api.h"


#define XBEE_OFF          (PORTB |= (1<<PB4))
#define XBEE_ON         (PORTB &= ~(1<<PB4))
#define XBEE_CONFIG      (DDRB |= (1<<PB4))

#define SENSOR_DELAY_8_4sec 0
#define SENSOR_DELAY_5sec 104
#define SENSOR_DELAY_2_5sec 180
#define SENSOR_DELAY_1_8sec 200

#define VREF_VCC  0
#define VREF_11  (1<<REFS1)
#define VREF_256 (1<<REFS1) | (1<<REFS2)

void init_adc(void) {
    ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1); // prescale down to 125khz for accuracy
    ADMUX = VREF_256 | 0x03;  // vcc reference, and source = ADC3
}

unsigned int adc_read(void)
{
    ADCSRA |= (1<<ADSC);               // begin the conversion
    while (ADCSRA & (1<<ADSC)) ;   // wait for the conversion to complete
    unsigned char lsb = ADCL;                              // read the LSB first
    return (ADCH << 8) | lsb;          // read the MSB and return 10 bit result
}

unsigned int init_adc_int_temp(void)
{
    ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1); // prescale down to 125khz for accuracy
    ADMUX = VREF_11 | (1<<MUX3) | (1<<MUX2) | (1<<MUX1) | (1<<MUX0); // internal temp sensor
}


void init(void) {
	XBEE_CONFIG;
	XBEE_OFF;
	clock_prescale_set(0);
	TX_CONFIG;
	// things we never need...
	power_timer1_disable();
	power_usi_disable();
	
	// setup the timer0 prescalar stuff
	GTCCR |= (1<<TSM) | (1<<PSR0);  // reset prescalar
	TCCR0B |= (1<<CS02) | (1<<CS00);  // timer prescale down to 1024
	GTCCR &= ~(1<<TSM);  // allow prescalar changes to take effect
	
}

void s_print_header(void) {
	putChar('x');
	putChar('x');
	putChar('x');
}

void s_print_short(unsigned int val) {
	putChar((unsigned char)(val >> 8));
	putChar((unsigned char)(val & 0xff));
}

// Handle printing the sensor type opcode into the packet
void s_print_sensor(unsigned char sensor_type, unsigned int value) {
	putChar(sensor_type);
	s_print_short(value);
}


int main(void) {
	init();

	kpacket packet;
	packet.header = 'x';

	while (1) {
		power_adc_enable();
		init_adc();
		unsigned int sensor1 = adc_read();
		init_adc_int_temp();
		unsigned int sensor2 = adc_read();
		power_adc_disable();
		packet.type1 = 36;
		packet.value1 = sensor1;
		packet.type2 = 'i';
		packet.value2 = sensor2;
		XBEE_ON;
		_delay_ms(15);  // xbee manual says 2ms for sleep mode 2, 13 for sleep mode 1
		xbee_send_16(1, packet);  // manually set my base station to address MY = 1
		XBEE_OFF;

		// slow down the clocks, so we can do a wakeup in ~5 seconds from one timer overflow..
		clock_prescale_set(8);  // fclock now 1Mhz
		// preload the timer so it overflows about where we expect
		TCNT0 = SENSOR_DELAY_5sec;
		TIMSK |= (1<<TOIE0);  // enable timer0 overflow interrupt (wakeup)
		

		// now sleep!
		set_sleep_mode(0);
		sleep_enable();
		sei();
		sleep_cpu();
		sleep_disable();
		cli();
		clock_prescale_set(0);
	}
}

