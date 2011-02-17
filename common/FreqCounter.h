
/*
  FreqCounter.h - Library for a Frequency Counter c.
  Created by Martin Nawrath, KHM Lab3, Dec. 2008
  Released into the public domain.
*/



#ifndef FreqCounter_h
#define FreqCounter_h

#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

#include <avr/interrupt.h>
#include <util/delay.h>


	extern volatile unsigned long f_freq;
	extern volatile unsigned char f_ready;
	extern volatile unsigned char f_mlt;
	extern volatile unsigned int f_tics;
	extern volatile unsigned int f_period;
	extern volatile unsigned int f_comp;
	
	void FreqCounter__start(unsigned int comp, int ms);
	unsigned long blockingRead(unsigned int comp, int ms);
	
	

#endif





