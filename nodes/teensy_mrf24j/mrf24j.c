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
volatile uint8_t last_interrupt;

ISR(INT0_vect) {
    // read and clear from the radio
    last_interrupt = mrf_read_short(MRF_INTSTAT);
    if (last_interrupt & MRF_I_RXIF) {
        gotrx = 1;
    }
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
        phex(last_interrupt);
        if (gotrx) {
            LED_ON;
            gotrx = 0;
            last_interrupt = 0;
            // skip the full datasheet recommended read cycle fr now, just dump the entire packet buffer
            cli();
            print("Received a packet!\n\r");
            mrf_write_short(MRF_BBREG1, 0x04);  // RXDECINV - disable receiver

            uint8_t frame_length = mrf_read_long(0x300);  // read start of rxfifo for
            phex(frame_length);
            print("\r\nPacket data:\r\n");
            for (int i = 1; i <= frame_length; i++) {
                tmp = mrf_read_long(0x300 + i);
                phex(tmp);
            }
            print("\r\nLQI/RSSI=");
            uint8_t lqi = mrf_read_long(0x300 + frame_length + 1);
            uint8_t rssi = mrf_read_long(0x300 + frame_length + 2);
            phex(lqi);
            phex(rssi);

            mrf_write_short(MRF_BBREG1, 0x00);  // RXDECINV - enable receiver
            sei();
            LED_OFF;
        } else {
            LED_OFF;
        }
        _delay_ms(250);
    }
}

