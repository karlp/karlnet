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


#define MRF_RXMCR 0x00
#define MRF_PANIDL 0x01
#define MRF_PANIDH 0x02
#define MRF_SADRL 0x03
#define MRF_SADRH 0x04
#define MRF_EADR0 0x05
#define MRF_EADR1 0x06
#define MRF_EADR2 0x07
#define MRF_EADR3 0x08
#define MRF_EADR4 0x09
#define MRF_EADR5 0x0A
#define MRF_EADR6 0x0B
#define MRF_EADR7 0x0C
#define MRF_RXFLUSH 0x0D
//#define MRF_Reserved 0x0E
//#define MRF_Reserved 0x0F
#define MRF_ORDER 0x10
#define MRF_TXMCR 0x11
#define MRF_ACKTMOUT 0x12
#define MRF_ESLOTG1 0x13
#define MRF_SYMTICKL 0x14
#define MRF_SYMTICKH 0x15
#define MRF_PACON0 0x16
#define MRF_PACON1 0x17
#define MRF_PACON2 0x18
//#define MRF_Reserved 0x19
#define MRF_TXBCON0 0x1A
#define MRF_TXNCON 0x1B
#define MRF_TXG1CON 0x1C
#define MRF_TXG2CON 0x1D
#define MRF_ESLOTG23 0x1E
#define MRF_ESLOTG45 0x1F
#define MRF_ESLOTG67 0x20
#define MRF_TXPEND 0x21
#define MRF_WAKECON 0x22
#define MRF_FRMOFFSET 0x23
#define MRF_TXSTAT 0x24
#define MRF_TXBCON1 0x25
#define MRF_GATECLK 0x26
#define MRF_TXTIME 0x27
#define MRF_HSYMTMRL 0x28
#define MRF_HSYMTMRH 0x29
#define MRF_SOFTRST 0x2A
//#define MRF_Reserved 0x2B
#define MRF_SECCON0 0x2C
#define MRF_SECCON1 0x2D
#define MRF_TXSTBL 0x2E
//#define MRF_Reserved 0x2F
#define MRF_RXSR 0x30
#define MRF_INTSTAT 0x31
#define MRF_INTCON 0x32
#define MRF_GPIO 0x33
#define MRF_TRISGPIO 0x34
#define MRF_SLPACK 0x35
#define MRF_RFCTL 0x36
#define MRF_SECCR2 0x37
#define MRF_BBREG0 0x38
#define MRF_BBREG1 0x39
#define MRF_BBREG2 0x3A
#define MRF_BBREG3 0x3B
#define MRF_BBREG4 0x3C
//#define MRF_Reserved 0x3D
#define MRF_BBREG6 0x3E
#define MRF_CCAEDTH 0x3F

#define MRF_RFCON0 0x200
#define MRF_RFCON1 0x201
#define MRF_RFCON2 0x202
#define MRF_RFCON3 0x203
#define MRF_RFCON5 0x205
#define MRF_RFCON6 0x206
#define MRF_RFCON7 0x207
#define MRF_RFCON8 0x208
#define MRF_SLPCAL0 0x209
#define MRF_SLPCAL1 0x20A
#define MRF_SLPCAL2 0x20B
#define MRF_RSSI 0x210
#define MRF_SLPCON0 0x211
#define MRF_SLPCON1 0x220
#define MRF_WAKETIMEL 0x222
#define MRF_WAKETIMEH 0x223
#define MRF_REMCNTL 0x224
#define MRF_REMCNTH 0x225
#define MRF_MAINCNT0 0x226
#define MRF_MAINCNT1 0x227
#define MRF_MAINCNT2 0x228
#define MRF_MAINCNT3 0x229
#define MRF_TESTMODE 0x22F
#define MRF_ASSOEADR1 0x231
#define MRF_ASSOEADR2 0x232
#define MRF_ASSOEADR3 0x233
#define MRF_ASSOEADR4 0x234
#define MRF_ASSOEADR5 0x235
#define MRF_ASSOEADR6 0x236
#define MRF_ASSOEADR7 0x237
#define MRF_ASSOSADR0 0x238
#define MRF_ASSOSADR1 0x239
#define MRF_UPNONCE0 0x240
#define MRF_UPNONCE1 0x241
#define MRF_UPNONCE2 0x242
#define MRF_UPNONCE3 0x243
#define MRF_UPNONCE4 0x244
#define MRF_UPNONCE5 0x245
#define MRF_UPNONCE6 0x246
#define MRF_UPNONCE7 0x247
#define MRF_UPNONCE8 0x248
#define MRF_UPNONCE9 0x249
#define MRF_UPNONCE10 0x24A
#define MRF_UPNONCE11 0x24B
#define MRF_UPNONCE12 0x24C



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

void mrf_set_interrupts(void) {}

void mrf_set_channel(void) {}

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



int main(void) {
    init();

    print("woke up...woo\n");
    uint8_t tmp;
    mrf_reset();
    _delay_ms(100);

    mrf_init();

    mrf_pan_write(0xcafe);
    _delay_ms(500);
    while (1) {
        // start by reading all control registers
        uint16_t pan = mrf_pan_read();
        phex16(pan);
        tmp = mrf_read_short(MRF_TXMCR);
        phex(tmp);
        print("\r\n");
        _delay_ms(500);
    }
}

