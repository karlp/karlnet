// Karl Palsson, 2011
// BSD/MIT license where possible or not otherwise specified

#include "lib_mrf24j.h"
#include <avr/io.h>
#include <util/delay.h>



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

uint8_t mrf_read_long(uint16_t address) {
    MRF_SELECT;
    uint8_t ahigh = address >> 3;
    uint8_t alow = address << 5;
    spi_tx(0x80 | ahigh);  // high bit for long
    spi_tx(alow);
    spi_tx(0);
    MRF_DESELECT;
    return SPDR;
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

