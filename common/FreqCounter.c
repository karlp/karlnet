/*
  FreqCounter.h - 
  Using Counter1 for counting Frequency on T1 / PD5 / digitalPin 5 
  Using Timer2 for Gatetime generation

  Martin Nawrath KHM LAB3
  Kunsthochschule f�r Medien K�ln
  Academy of Media Arts
  http://www.khm.de
  http://interface.khm.de/index.php/labor/experimente/
  
  History:
  	Dec/08 - V0.0 
        2010/03  Added mega32u4 support... Karl Palsson

	

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/



#include "FreqCounter.h"


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

    // all devices use 16 bit timer1 for counting...
    // Set up timer1
    TCCR1A = 0;
    // reset timer1 to external clock source on T1, rising edge.
    TCCR1B = (1 << CS10) | (1 << CS11) | (1 << CS12);

#if defined (__AVR_ATmega168__) || defined (__AVR_ATmega328P__)
	// hardware counter setup ( refer atmega168.pdf chapter 16-bit counter1)
  TCNT1=0;           		// counter value = 0
  // timer2 setup / is used for frequency measurement gatetime generation
  // timer 2 presaler set to 256 / timer 2 clock = 16Mhz / 256 = 62500 Hz
  TCCR2A=0;
  TCCR2B=0;
  cbi (TCCR2B ,CS20);
  sbi (TCCR2B ,CS21);
  sbi (TCCR2B ,CS22);

  //set timer2 to CTC Mode
  cbi (TCCR2A ,WGM20);  
  sbi (TCCR2A ,WGM21);
  cbi (TCCR2B ,WGM22);
  OCR2A = 124;   


  sbi (GTCCR,PSRASY);       // reset presacler counting
  TCNT2=0;                  // timer2=0
  TCNT1=0;                  // Counter1 = 0

  cbi (TIMSK0,TOIE0);       // disable Timer0  //disable  millis and delay
  sbi (TIMSK2,OCIE2A);      // enable Timer2 Interrupt

  TCCR1B = TCCR1B | 7;      //  Counter Clock source = pin T1 , start counting now
#elif defined (__AVR_ATmega32U4__)

  // TODO - try and merge this with tiny84

    // mega32u4 doesn't have timer2, but it has timer0 as an 8bit counter...
    // Setup timer0 as an 8 bit gate counter...
    // we want 2ms or so,
    TCCR0A = (1<<WGM01);  // CTC mode, normal (no Output compare)
    GTCCR |= (1<<PSRASY);  // reset prescaler (undocumented bit in mega32u4?)
    TCCR0B = (1<<CS02);  // timer0 on clk/256 == 16MHZ / 256 = 62500hz..
    OCR0A = 124;  // 125 counts of 62500 Hz before an output compare interrupt...

    TCNT1 = 0;
    TCNT0 = 0;

    TIMSK0 = (1<<OCIE0A);
#elif defined (__AVR_ATtiny84__)
    // Setup timer0 as an 8 bit gate counter...
    // we want 2ms or so,
    TCCR0A = (1 << WGM01); // CTC mode, normal (no Output compare)
    TCCR0B = (1 << CS01) | (1 << CS00); // timer 0 on clk/64 == (Mhz / 64 = 125khz
    OCR0A = 249; // 250 counts of 125khz equals 2ms

    // reset gate counter, unmask it's interrupt, starting both timers
    TCNT1 = 0;
    TCNT0 = 0;
    TIMSK0 = (1 << OCIE0A);

#endif
}

unsigned long blockingRead(unsigned int comp, int ms) {
	FreqCounter__start(comp, ms);
	while (f_ready == 0) {
		;
	}          // wait until counter ready
	return f_freq;    
}



//******************************************************************
//  Timer2 Interrupt Service is invoked by hardware Timer2 every 2ms = 500 Hz
//  16Mhz / 256 / 125 = 500 Hz
//  here the gatetime generation for freq. measurement takes place: 

#if defined (__AVR_ATmega168__) || defined (__AVR_ATmega328P__)
ISR(TIMER2_COMPA_vect) {
#define GATE_CONTROL        TIMSK2
#define GATE_INTERRUPT_FLAG OCIE2A
#elif defined (__AVR_ATmega32U4__)
ISR(TIMER0_COMPA_vect) {
#define GATE_CONTROL        TIMSK0
#define GATE_INTERRUPT_FLAG OCIE0A
#elif  defined (__AVR_ATtiny84__)
ISR(TIM0_COMPA_vect) {
#define GATE_CONTROL        TIMSK0
#define GATE_INTERRUPT_FLAG OCIE0A
#else
#error You need one of them?!
#endif

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
