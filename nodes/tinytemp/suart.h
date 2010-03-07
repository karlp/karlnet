// Karl Palsson, 2010
// header for a polled software, tx only, uart

// PUBLIC CONFIG
#define TX_PIN 		PINB1
#define TX_PORT  	PORTB
#define TX_CONFIG_PORT  DDRB
//#define BAUD 9600
#define BAUD 19200


// internal config
#define TX_HIGH		(TX_PORT |= (1<<TX_PIN))
#define TX_LOW		(TX_PORT &= ~(1<<TX_PIN))

#define BIT_DELAY_USEC ( 1000000  / BAUD)


// PUBLIC METHODS
#define TX_CONFIG 	(TX_CONFIG_PORT |= (1<<TX_PIN))  // call this in your init...
void putChar(unsigned char txb);

