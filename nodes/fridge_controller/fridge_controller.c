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

// USCK and DO pins are output
#define SPI_CONFIG  (DDRA |= (1<<PINA5) | (1<<PINA4))

#define RELAY_ENABLE (DDRA |= (1<<PINA2))
#define RELAY_STATE (PORTA & (1<<PINA2))
#define RELAY_OFF (PORTA &= ~(1<<PINA2))
#define RELAY_ON  (PORTA |= (1<<PINA2))

#define MRF_DDR (DDRB)
#define MRF_PORT (PORTB)
#define MRF_PIN_RESET   (PINB1)
#define MRF_PIN_CS      (PINB0)
#define MRF_PIN_INT     (PINB2)

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
    MRF_DDR = (1<<MRF_PIN_RESET) | (1<<MRF_PIN_CS);
    SPI_CONFIG;

    // interrupt pin from mrf, low level triggered
    //EIMSK |= (1<<INT0);
    GIMSK |= (1<<INT0);
    RELAY_ENABLE;
    init_adc();
    init_slow_timer();
}

volatile uint16_t slowticker;

ISR(TIM1_COMPA_vect) {
    slowticker++;
    // make sure it's working...
}

volatile uint8_t gotrx;
volatile uint8_t txok;
volatile uint8_t last_interrupt;

ISR(INT0_vect) {
    // read and clear from the radio
    last_interrupt = mrf_read_short(MRF_INTSTAT);
    if (last_interrupt & MRF_I_RXIF) {
        gotrx = 1;
    }
    if (last_interrupt & MRF_I_TXNIF) {
        txok = 1;
    }
}

int main(void) {
    init();

    mrf_reset(&MRF_PORT, MRF_PIN_RESET);

    mrf_init();

    mrf_pan_write(0xcafe);
    mrf_address16_write(0x6002);
    //mrf_write_short(MRF_RXMCR, 0x01); // promiscuous!
    sei();
    char buf[2];
    while (1) {

        // Read temp/pot
        uint16_t currentValue = adc_read();

        if (slowticker > 0) {
            slowticker = 0;
            buf[0] = currentValue >> 8;
            buf[1] = currentValue & 0x0ff;
            mrf_send16(0x4202, 2, buf);

            if (RELAY_STATE) {
                RELAY_OFF;
            } else {
                RELAY_ON;
            }
        } 
        //_delay_ms(100);
    }
}

