// Karl Palsson, 2010
// 
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h> 
#include <avr/pgmspace.h>
#include <avr/power.h>
#include <util/delay.h>
// usb hid...
#include "usb_debug_only.h"
#include "print.h"
// pjrc uart!
#include "pjrc_uart.h"

#include "karlnet.h"
#include "xbee-api.h"
#include "FreqCounter.h"

// from teensy code samples...
// NOTE! If you're using the frequency counter for the humidity sensor, you CANNOT use the onboard LED!
#define LED_ON          (PORTD |= (1<<6))
#define LED_OFF         (PORTD &= ~(1<<6))
#define LED_CONFIG      (DDRD |= (1<<6))

#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))

#define ADC_ENABLE  (ADCSRA |= (1<<ADEN))
#define ADC_DISABLE  (ADCSRA &= ~(1<<ADEN))


#define BAUD_RATE 19200

#define VREF_256 (1<<REFS1) | (1<<REFS0)  // requires external cap at aref!

void init_adc(void) {
    ADMUX = VREF_256 | 0;  // adc0
}

void init_adc_int_temp(void)
{
    ADMUX = VREF_256 | (1<<MUX5) | (1<<MUX2) | (1<<MUX1) | (1<<MUX0); // internal temp sensor
}

unsigned int adc_read(void)
{
    ADCSRA |= (1<<ADSC);               // begin the conversion
    while (ADCSRA & (1<<ADSC)) ;   // wait for the conversion to complete
    unsigned char lsb = ADCL;                              // read the LSB first
    return (ADCH << 8) | lsb;          // read the MSB and return 10 bit result
}

unsigned int readSensorFreq(void) {
        // this gives you an answer in hz, but only up to 32k :)
        return blockingRead(1, 1000);
}



void init(void) {

	usb_init();
	
	uart_init(BAUD_RATE);

        ADCSRA = (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);  // enable ADC

	while (!usb_configured()) {
	}
	// Advised to wait a bit longer by pjrc.com
	// as not all drivers will be ready even after host is up
	_delay_ms(500);
}


int main(void) {
	CPU_PRESCALE(0);
	init();

        kpacket packet;
        packet.header = 'x';
        packet.version = 1;
        packet.nsensors = 3;

	print("woke up...woo\n");
        sei();
	while (1) {
                power_adc_enable();
                _delay_us(270);  // not mentioned in 32u4 datasheet, value from tiny85 and mega328p
                ADC_ENABLE;
                init_adc();
                unsigned int sensor1 = adc_read();
                sensor1 = adc_read();
                sensor1 = adc_read();

                init_adc_int_temp();
                unsigned int sensor2 = adc_read();
                ADC_DISABLE;
                power_adc_disable();

                unsigned int freq1 = readSensorFreq();

                ksensor s1 = {36, sensor1};
                ksensor s2 = {'f', freq1};
                ksensor s3 = {'I', sensor2};
                packet.ksensors[0] = s1;
                packet.ksensors[1] = s2;
                packet.ksensors[2] = s3;

                xbee_send_16(1, packet);

                _delay_ms(4000);
	}
}

