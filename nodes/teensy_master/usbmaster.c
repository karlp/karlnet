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
// standard avr uart
#include "uart.h"

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
        // this will ABSOLUTELY not work with rx rf data streaming in!
        // the rx ring buffer will massively overflow!
        return blockingRead(1, 1000);
}



void init(void) {

	usb_init();
        uart_init(UART_BAUD_SELECT(BAUD_RATE, F_CPU));

        ADCSRA = (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);  // enable ADC

	while (!usb_configured()) {
	}
	// Advised to wait a bit longer by pjrc.com
	// as not all drivers will be ready even after host is up
	_delay_ms(500);
        ADC_ENABLE;
        init_adc();
}


int main(void) {
	CPU_PRESCALE(0);
	init();

	print("woke up...woo\n");
	while (1) {

            // For starters,  just use the uart library, and do the same as before, echo it all...
            uint16_t rxc = uart_getc();
            while (rxc == UART_NO_DATA) {
                rxc = uart_getc();
            }
            usb_debug_putchar(rxc & 0xff);
            // is there anyway I can not implement xbee packet parsing here?
            // Can I use usb magic endpoints to send my local station "out of band" ?

	}
}

