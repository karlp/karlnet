// Karl Palsson, 2011
// BSD/MIT license where possible or not otherwise specified

#include "lib_mrf24j.h"
#include <avr/io.h>
#include <util/delay.h>


/**
 * use with mrf_reset(&PORTB, PINB5);
 */
void mrf_reset(volatile uint8_t *port, uint8_t pin) {
    *port &= ~(1 << pin);
    _delay_ms(10);  // just my gut
    *port |= (1 << pin);  // active low biatch!
    _delay_ms(20);  // from manual
}


/**
 * Internal spi handling, works on both avr tiny, with USI,
 * and also regular hardware SPI.
 *
 * For regular hardware spi, requires the spi hardware to already be setup!
 * (TODO, you can handle that yourself, or even, have a compile time flag that
 * determines whether to use internal, or provided spi_tx/rx routines)
 */
uint8_t spi_tx(uint8_t cData) {

#if defined(SPDR)
    // AVR platforms with "regular" SPI hardware

    /* Start transmission */
    SPDR = cData;
    /* Wait for transmission complete */
    loop_until_bit_is_set(SPSR, SPIF);
    return SPDR;
#elif defined (USIDR)
    // AVR platforms with USI interfaces, capable of SPI
        /* Start transmission */
    USIDR = cData;
    USISR = (1 << USIOIF);
    do {
        USICR = (1 << USIWM0) | (1 << USICS1) | (1 << USICLK) | (1 << USITC);
    } while ((USISR & (1 << USIOIF)) == 0);
    return USIDR;
//#else - stupid netbeans doesn't find the right defines :(
//#error "You don't seem to have any sort of spi hardware!"
#endif
}


uint8_t mrf_read_short(uint8_t address) {
    MRF_SELECT;
    // 0 top for short addressing, 0 bottom for read
    spi_tx(address<<1 & 0b01111110);
    uint8_t res = spi_tx(0x0);
    MRF_DESELECT;
    return res;
}

uint8_t mrf_read_long(uint16_t address) {
    MRF_SELECT;
    uint8_t ahigh = address >> 3;
    uint8_t alow = address << 5;
    spi_tx(0x80 | ahigh);  // high bit for long
    spi_tx(alow);
    uint8_t res = spi_tx(0);
    MRF_DESELECT;
    return res;
}


void mrf_write_short(uint8_t address, uint8_t data) {
    MRF_SELECT;
    // 0 for top address, 1 bottom for write
    spi_tx((address<<1 & 0b01111110) | 0x01);
    spi_tx(data);
    MRF_DESELECT;
}

void mrf_write_long(uint16_t address, uint8_t data) {
    MRF_SELECT;
    uint8_t ahigh = address >> 3;
    uint8_t alow = address << 5;
    spi_tx(0x80 | ahigh);  // high bit for long
    spi_tx(alow | 0x10);  // last bit for write
    spi_tx(data);
    MRF_DESELECT;
}

uint16_t mrf_pan_read(void) {
    uint8_t panh = mrf_read_short(MRF_PANIDH);
    return panh << 8 | mrf_read_short(MRF_PANIDL);
}

void mrf_pan_write(uint16_t panid) {
    mrf_write_short(MRF_PANIDH, panid >> 8);
    mrf_write_short(MRF_PANIDL, panid & 0xff);
}

void mrf_address16_write(uint16_t address16) {
    mrf_write_short(MRF_SADRH, address16 >> 8);
    mrf_write_short(MRF_SADRL, address16 & 0xff);
}

uint16_t mrf_address16_read(void) {
    uint8_t a16h = mrf_read_short(MRF_SADRH);
    return a16h << 8 | mrf_read_short(MRF_SADRL);
}

/**
 * Simple send 16, with acks, not much of anything.. assumes src16 and local pan only.
 * @param data
 */
void mrf_send16(uint16_t dest16, uint8_t len, char * data) {

    int i = 0;
    mrf_write_long(i++, 9);  // header length
    mrf_write_long(i++, 9+2+len); //+2 is because module seems to ignore 2 bytes after the header?!

// 0 | pan compression | ack | no security | no data pending | data frame[3 bits]
    mrf_write_long(i++, 0b01100001); // first byte of Frame Control
// 16 bit source, 802.15.4 (2003), 16 bit dest,
    mrf_write_long(i++, 0b10001000); // second byte of frame control
    mrf_write_long(i++, 1);  // sequence number 1

    uint16_t panid = mrf_pan_read();

    mrf_write_long(i++, panid & 0xff);  // dest panid
    mrf_write_long(i++, panid >> 8);
    mrf_write_long(i++, dest16 & 0xff);  // dest16 low
    mrf_write_long(i++, dest16 >> 8); // dest16 high

    uint16_t src16 = mrf_address16_read();
    mrf_write_long(i++, src16 & 0xff); // src16 low
    mrf_write_long(i++, src16 >> 8); // src16 high

    i+=2;  // All testing seems to indicate that the next two bytes are ignored.
    for (int q = 0; q < len; q++) {
        mrf_write_long(i++, data[q]);
    }
    // ack on, and go!
    mrf_write_short(MRF_TXNCON, (1<<MRF_TXNACKREQ | 1<<MRF_TXNTRIG));
}

void mrf_set_interrupts(void) {
    // interrupts for rx and tx normal complete
    mrf_write_short(MRF_INTCON, 0b11110110);
}

// Set the channel to 12, 2.41Ghz, xbee channel 0xC
void mrf_set_channel(void) {
    mrf_write_long(MRF_RFCON0, 0x13);
}

void mrf_init(void) {
/*
 // Seems a bit ridiculous when I use reset pin anyway
    mrf_write_short(MRF_SOFTRST, 0x7); // from manual
    while (mrf_read_short(MRF_SOFTRST) & 0x7 != 0) {
        ; // wait for soft reset to finish
    }
*/
    mrf_write_short(MRF_PACON2, 0x98); // – Initialize FIFOEN = 1 and TXONTS = 0x6.
    mrf_write_short(MRF_TXSTBL, 0x95); // – Initialize RFSTBL = 0x9.

    mrf_write_long(MRF_RFCON0, 0x03); // – Initialize RFOPT = 0x03.
    mrf_write_long(MRF_RFCON1, 0x01); // – Initialize VCOOPT = 0x02.
    mrf_write_long(MRF_RFCON2, 0x80); // – Enable PLL (PLLEN = 1).
    mrf_write_long(MRF_RFCON6, 0x90); // – Initialize TXFIL = 1 and 20MRECVR = 1.
    mrf_write_long(MRF_RFCON7, 0x80); // – Initialize SLPCLKSEL = 0x2 (100 kHz Internal oscillator).
    mrf_write_long(MRF_RFCON8, 0x10); // – Initialize RFVCO = 1.
    mrf_write_long(MRF_SLPCON1, 0x21); // – Initialize CLKOUTEN = 1 and SLPCLKDIV = 0x01.

    //  Configuration for nonbeacon-enabled devices (see Section 3.8 “Beacon-Enabled and Nonbeacon-Enabled Networks”):
    mrf_write_short(MRF_BBREG2, 0x80); // Set CCA mode to ED
    mrf_write_short(MRF_CCAEDTH, 0x60); // – Set CCA ED threshold.
    mrf_write_short(MRF_BBREG6, 0x40); // – Set appended RSSI value to RXFIFO.
    mrf_set_interrupts();
    mrf_set_channel();
    // max power is by default.. just leave it...
    //Set transmitter power - See “REGISTER 2-62: RF CONTROL 3 REGISTER (ADDRESS: 0x203)”.
    mrf_write_short(MRF_RFCTL, 0x04); //  – Reset RF state machine.
    mrf_write_short(MRF_RFCTL, 0x00); // part 2
    _delay_us(500); // delay at least 192usec
}

