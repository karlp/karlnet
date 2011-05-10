// Karl Palsson, 2011
// 
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h> 
#include <avr/pgmspace.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "lib_mrf24j.h"

#define TICKS_PER_500MS 15625

#define SPI_CONFIG  (DDRA = (1<<DO) | (1<<DI) | (1<<USCK))


void init_adc(void) {
    ADMUX = 0;  // vcc aref, adc0.
    ADCSRA |= (1<<ADEN);
}

/**
 * Set up a 500ms per tick timer, using Timer 1
 */
void init_slow_timer(void) {
    // 8mhz internal oscillator / 1024 ==> 7812hz
    // plus clear timer on compare match.
    OCR1A = TICKS_PER_500MS;
    TCCR1B = (1<<WGM12) | (1<<CS12) | (1<<CS10);
    //TCNT1 = 0;
    TIMSK1 = (1<<OCIE1A);
}

uint16_t adc_read(void)
{
    ADCSRA |= (1<<ADSC);               // begin the conversion
    while (ADCSRA & (1<<ADSC)) ;   // wait for the conversion to complete
    unsigned char lsb = ADCL;                              // read the LSB first
    return (ADCH << 8) | lsb;          // read the MSB and return 10 bit result
}


void SPI_MasterInit(void) {
}

void init(void) {
//    MRF_CONFIG;
//    SPI_CONFIG;
    //SPI_MasterInit();

    // interrupt pin from mrf
    //EIMSK |= (1<<INT0);
    DDRB = (1<<PINB2);
    init_adc();
    init_slow_timer();

}

void spi_tx(uint8_t cData) {
    /* Start transmission */
    USIDR = cData;
    USISR = (1<<USIOIF);
    /* Wait for transmission complete */
    while (!(USISR & (1 << USIOIF)))
        // while not done, keep hitting the clock toggle bit?
        USICR = (1<<USITC);
}

volatile uint16_t slowticker;

ISR(TIM1_COMPA_vect) {
    slowticker++;
    // make sure it's working...
    if (PORTB & (1<<PINB2)) {
        PORTB &= ~(1<<PINB2);
    } else {
        PORTB |= (1<<PINB2);
    }
}

int main(void) {
    init();

    sei();
    while (1) {

        // Read temp/pot
        uint16_t currentValue = adc_read();

        uint8_t state;
        uint8_t done;
        if (slowticker % 4 == 0) {
            // Will keep getting hit repeatedly here, don't use it like this.
        } 
        //_delay_ms(100);
    }
}

