// Karl Palsson, 2011
// 
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h> 
#include <avr/pgmspace.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <util/delay.h>
// usb hid...
#include "usb_debug_only.h"
#include "print.h"
#include "lib_mrf24j.h"


// from teensy code samples...
// NOTE! If you're using the frequency counter for the humidity sensor, you CANNOT use the onboard LED!
#define LED_ON          (PORTD |= (1<<6))
#define LED_OFF         (PORTD &= ~(1<<6))
#define LED_CONFIG      (DDRD |= (1<<6))


#define DDR_SPI     DDRB
#define MISO        PINB3
#define MOSI        PINB2
#define SCLK        PINB1

#define SPI_CONFIG  (DDR_SPI = (1<<MOSI) | (1<<SCLK) | (1<<PINB0))


void SPI_MasterInit(void) {
    /* Enable SPI, Master, set clock rate fck/4 */
    SPCR = (1 << SPE) | (1 << MSTR);
    // So, that's either 4Mhz, or 2Mhz, depending on whether Fosc is
    // 16mhz crystal, or 8mhz clock prescaled?
    // If I can read the mrf24j sheet properly, this should work right
    // up to 10Mhz, but let's not push it.... (yet)
}

void init(void) {
    clock_prescale_set(clock_div_2); // we run at 3.3v
    usb_init();
    MRF_CONFIG;
    SPI_CONFIG;
    LED_CONFIG;
    SPI_MasterInit();

    // interrupt pin from mrf
    EIMSK |= (1<<INT0);

    while (!usb_configured()) {
    }
    // Advised to wait a bit longer by pjrc.com
    // as not all drivers will be ready even after host is up
    _delay_ms(500);
}

void spi_tx(uint8_t cData) {
    /* Start transmission */
    SPDR = cData;
    /* Wait for transmission complete */
    while (!(SPSR & (1 << SPIF)))
        ;
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

void mrf_tx(void) {
    // Fill tx normal fifo with data,
    // 1 byte header length first (m)
    // 1 byte frame length (m+n)
    // then header (m)
    // then data (n)

    // set acknowledgements on
    // mrf_write_short(TXNCON, 0x04);
//#define FC_TX_NORMAL  (0b00100000)   // let's seee which bit order we're in...
    
// let's see how bit orders go...
// data frame, no ack, no data pending, no security, pan id compression
#define FC_BYTE0 (0b01000001) // or the other bit order
// let's skip
    mrf_write_long(0x02, FC_BYTE0); // first byte of Frame Control

// 16 bit source, version 1, 16 bit dest,
#define FC_BYTE1 (0b10011000)
    mrf_write_long(0x03, FC_BYTE1); // second byte of frame control
    mrf_write_long(0x04, 1);  // sequence number 1
    mrf_write_long(0x05, 0xfe);  // dest panid
    mrf_write_long(0x06, 0xca);
    mrf_write_long(0x07, 0x02);  // dest 16high
    mrf_write_long(0x08, 0x42); // dest 16low
    mrf_write_long(0x09, 0x01); // src16 low
    mrf_write_long(0x0a, 0x60); // src16 high
    mrf_write_long(0x00, 9);  // header length
    //mrf_write_long(0x01, 9+4); // header + data
    mrf_write_long(0x01, 9+4+2); // header + data?
    // seems I've got header calculation lengthw rong somewhere?

    // xbee sees this as valid, but only seems to see 'c' and 'd', not a and b
    mrf_write_long(0x0b, 'a');
    mrf_write_long(0x0c, 'b');
    mrf_write_long(0x0d, 'c');
    mrf_write_long(0x0e, 'd');
    mrf_write_long(0x0f, 'e');
    mrf_write_long(0x10, 'f');


    mrf_write_short(MRF_TXNCON, 0x01); // set tx trigger bit, will be cleared by hardware

}

int main(void) {
    init();

    print("woke up...woo\n");
    uint8_t tmp;
    mrf_reset();

    mrf_init();

    mrf_pan_write(0xcafe);
    mrf_address16_write(0x6001);
    //mrf_write_short(MRF_RXMCR, 0x01); // promiscuous!
    sei();
    while (1) {
        print ("txxxing...\r\n");
        mrf_tx();
        if (txok) {
            print("tx went ok...\r\n");
        }
        _delay_ms(250);
    }
}

