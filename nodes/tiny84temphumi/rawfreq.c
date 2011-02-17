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

#define XBEE_OFF        (PORTA |= (1<<PINA7))
#define XBEE_ON         (PORTA &= ~(1<<PINA7))
#define DI_ON        (PORTA |= (1<<PINA6))
#define DI_OFF         (PORTA &= ~(1<<PINA6))
#define XBEE_CONFIG     (DDRA |= (1<<PINA7))

#define ADC_ENABLE  (ADCSRA |= (1<<ADEN))
#define ADC_DISABLE  (ADCSRA &= ~(1<<ADEN))

#define PROBE_CHANNEL1_ENABLED_PIN      PINB0
#define PROBE_CHANNEL2_ENABLED_PIN      PINB1

#define BAUD_RATE 19200

void init(void) {
    clock_prescale_set(0);
    TX_CONFIG;
    // disable all digital input buffers, we only use analog inputs...
    //DIDR0 = 0xff;
    DDRB |= (1 << PB2); // enable output for OC0A
    DDRA |= (1<<PA6);  // enable output for
}



volatile unsigned long f_freq;

volatile unsigned char f_ready;
volatile unsigned char f_mlt;
volatile unsigned int f_tics;
volatile unsigned int f_period;
volatile unsigned int f_comp;

void FreqCounter__start(unsigned int comp, int ms) {

    f_period = ms / 2;
    f_comp = comp;

    f_ready = 0; // reset period measure flag
    f_tics = 0; // reset interrupt counter

    // mega32u4 doesn't have timer2, but it has timer0 as an 8bit counter...
    // same applies to attiny84
    //GTCCR |= (1 << PSR10); // reset prescaler

    // Setup timer0 as an 8 bit gate counter...
    // we want 2ms or so,
    TCCR0A = (1 << WGM01); // CTC mode, normal (no Output compare)
    //TCCR0A = (1 << COM0A0) | (1 << WGM01); // CTC mode, plus toggle OC0A (allows us to test the frequency we're running at
    TCCR0B = (1 << CS01) | (1 << CS00); // timer 0 on clk/64 == (Mhz / 64 = 125khz
    OCR0A = 249; // 250 counts of 125khz equals 2ms

    // reset gate counter, unmask it's interrupt
    TCNT0 = 0;
    TIMSK0 = (1 << OCIE0A);

    // Now start with timer1 for external events.
    TCCR1A = 0; // reset timer/counter1 control register A
    // reset timer1 to external clock source on T1, rising edge.
    // set timer/counter1 hardware as counter , counts events on pin T1 ( arduino pin 5)
    TCNT1 = 0;
    TCCR1B = (1 << CS10) | (1 << CS11) | (1 << CS12);

}


//******************************************************************
//  Timer2 Interrupt Service is invoked by hardware Timer2 every 2ms = 500 Hz
//  16Mhz / 256 / 125 = 500 Hz
//  here the gatetime generation for freq. measurement takes place:

ISR(TIM0_COMPA_vect) {
#define GATE_CONTROL        TIMSK0
#define GATE_INTERRUPT_FLAG OCIE0A
    // multiple 2ms = gate time = 100 ms
    if (f_tics >= f_period) {
        // end of gate time, measurement ready

        // GateCalibration Value, set to zero error with reference frequency counter
        _delay_us(f_comp); // 0.01=1/ 0.1=12 / 1=120 sec
        TCCR1B &= ~((1 << CS10) | (1 << CS11) | (1 << CS12)); // Stop event counter
        GATE_CONTROL &= ~(1 << GATE_INTERRUPT_FLAG); // disable gate count timer
        // FIXME - not needed for non-arduino - sbi(TIMSK0, TOIE0); // enable Timer0 again // millis and delay
        // calculate now frequeny value
        f_freq = 0x10000 * f_mlt; // mult #overflows by 65636
        f_freq += TCNT1; // add counter1 value
        f_mlt = 0;
        // duh, not until you're DONE calculating!
        f_ready = 1; // set global flag for end count period
    }
    f_tics++; // count number of interrupt events
    if (TIFR1 & 1) { // if Timer/Counter 1 overflow flag
        f_mlt++; // count number of Counter1 overflows
        TIFR1 |= (1<<TOV1); // clear timer1 overflow flag
    }
}

int main(void) {
    init();
    kpacket packet;
    packet.header = 'x';
    packet.version = 1;
    packet.nsensors = 3;

    sei();
    ksensor s1 = {0, 0};

    while (1) {
        XBEE_ON;
        DI_OFF;
        sei();
        FreqCounter__start(1, 1000); // start, gate time 1000ms

        ksensor s2 = {SENSOR_TEST, 0xaaaa};
        ksensor s3 = {SENSOR_TEST, 0xbbbb};

        packet.ksensors[1] = s2;
        packet.ksensors[2] = s3;


        while (f_ready == 0) {
            DI_ON;
        } // wait until counter ready
        cli();

        DI_OFF;

        uint32_t myfreq = f_freq;
        s1.type = FREQ_1SEC;
        s1.value = myfreq;
        packet.ksensors[0] = s1;

        xbee_send_16(1, packet);
        _delay_ms(4000);  // plus 1 second gating == 5 seconds per packet...
    }
}


