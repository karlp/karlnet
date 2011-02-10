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
#include "suart.h"
#include "karlnet.h"
#include "xbee-api.h"

//#include "FreqCounter.h"

#define XBEE_OFF        (PORTA |= (1<<PINA7))
#define XBEE_ON         (PORTA &= ~(1<<PINA7))
#define XBEE_CONFIG     (DDRA |= (1<<PINA7))

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

/*
unsigned long readSensorFreq(void) {
	// this gives you an answer in hz, but only up to 32k :)
	return blockingRead(1, 1000);
}
*/

void init(void) {
	XBEE_CONFIG;
        XBEE_OFF;
        clock_prescale_set(0);
        ADCSRA = (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);  // enable ADC

        TX_CONFIG;


        // turn off things we don't ever need...

        // turn off analog comparator
        ACSR &= ~(1<<ACIE);
        ACSR |= (1<<ACD);

        // disable all digital input buffers, we only use analog inputs...
        DIDR0 = 0xff;
        
/*
        power_timer0_disable();
        power_spi_disable();
        power_twi_disable();
*/

        set_sleep_mode(SLEEP_MODE_PWR_DOWN);  // deeeeeep sleep

        MCUSR &= ~(1<<WDRF);
        // begin timed sequence
        WDTCSR |= (1<<WDCE) | (1<<WDE);
        // set new watchdog timeout value
        WDTCSR = (1<<WDP3) | (1<<WDIE);  // 4 secs

}

EMPTY_INTERRUPT(WDT_vect);

int main(void) {
	init();
        kpacket packet;
        packet.header = 'x';
        packet.version = 1;
        packet.nsensors = 3;

        sei();
        unsigned long prior = 0;
	while (1) {
                power_adc_enable();
                _delay_us(70);  // bandgap wakeup time from datasheet (worst case)
                ADC_ENABLE;
                init_adc();
		unsigned int sensor1 = adc_read();
                ADC_DISABLE;
                power_adc_disable();
                
//		uint32_t freq1 = readSensorFreq();

                ksensor s1 = { 37, sensor1};
                ksensor s2 = { 'k', 0xaabbccdd};
                ksensor s3 = { 99, prior };
                packet.ksensors[0] = s1;
                packet.ksensors[1] = s2;
                packet.ksensors[2] = s3;

                TCCR1B = (1<<CS11);
                TCNT1 = 0;
		XBEE_ON;
		_delay_ms(2); // xbee manual says 2ms for sleep mode 2, 13 for sm1
                //xbee_send_16(1, packet);
                xbee_send_16(0x4202, packet);
		XBEE_OFF;
                prior = TCNT1;

                // Now sleeeep
                _delay_ms(3000);

                //sleep_mode();
                //sleep_disable();
	}
}

