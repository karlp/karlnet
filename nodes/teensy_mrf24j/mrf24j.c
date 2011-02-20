// Karl Palsson, 2011
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

// from teensy code samples...
// NOTE! If you're using the frequency counter for the humidity sensor, you CANNOT use the onboard LED!
#define LED_ON          (PORTD |= (1<<6))
#define LED_OFF         (PORTD &= ~(1<<6))
#define LED_CONFIG      (DDRD |= (1<<6))


#define DDR_MRF     DDRD
#define MRF_INT     PIND0
#define MRF_RESET   PIND1
#define MRF_CS      PIND2
#define MRF_SELECT  (PORTD &= ~(1<<MRF_CS))  // active low
#define MRF_DESELECT (PORTD |= (1<<MRF_CS))

#define DDR_SPI     DDRB
#define MISO        PINB3
#define MOSI        PINB2
#define SCLK        PINB1

#define MRF_CONFIG  (DDR_MRF = (1<<MRF_RESET) | (1<<MRF_CS))
#define SPI_CONFIG  (DDR_SPI = (1<<MOSI) | (1<<SCLK) | (1<<PINB0))



#define MRF_TXMCR   0x11

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

void mrf_reset(void) {
    PORTD &= ~(1<<MRF_RESET);
    _delay_ms(10);  // just my gut
    PORTD |= (1<<MRF_RESET);  // active low biatch!
    _delay_ms(20);  // from manual
}

uint8_t mrf_read_short(uint8_t address) {
    MRF_SELECT;
    // 0 top for short addressing, 0 bottom for read
    spi_tx(address<<1 & 0b01111110);
    spi_tx(0x0);
    MRF_DESELECT;
    return SPDR;
}

int main(void) {
    init();

    print("woke up...woo\n");
    uint8_t tmp;
    mrf_reset();

    while (1) {
        // start by reading all control registers

        for (uint8_t i = 0; i <= 0x3f; i++) {
            tmp = mrf_read_short(i);
            print("tmp");
            phex(i);
            print(" = ");
            phex(tmp);
            print(" ");
        }
        _delay_ms(500);
        print("\r\n");
    }
}

