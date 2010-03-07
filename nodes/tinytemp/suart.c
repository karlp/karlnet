// Karl Palsson, 2010
// Very basic polled IO, for a tx only software uart.
// config is in suart.h
#include <avr/io.h> 
#include <util/delay.h>
#include "suart.h"

// UGLY AS SHIT, but it works. fucking finally.
void putChar(unsigned char txb) {
	TX_LOW;  // start bit
	_delay_us(BIT_DELAY_USEC);
	for (int i = 0; i < 8; i++) {
		if (txb & 0x01) {
			TX_HIGH;
		} else {
			TX_LOW;
		}
		txb = txb >>1;
		_delay_us(BIT_DELAY_USEC);
	}
	TX_HIGH; // stop bit
	_delay_us(BIT_DELAY_USEC);
}

