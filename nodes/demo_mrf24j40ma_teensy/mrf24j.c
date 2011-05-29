// Karl Palsson, 2011
// false.ekta.is
// BSD/MIT Licensed.
#include <stdio.h>
#include <avr/io.h> 
#include <avr/pgmspace.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <util/delay.h>
// usb hid...
#include "usb_debug_only.h"
#include "lib_mrf24j.h"

#define MRF_DDR DDRB
#define MRF_PORT PORTB
#define MRF_PIN_RESET PINB4
#define MRF_PIN_CS PINB0  // This is also /SS
#define MRF_CONFIG  (MRF_DDR |= (1<<MRF_PIN_RESET) | (1<<MRF_PIN_CS))


#define SPI_DDR     DDRB
#define SPI_MISO        PINB3
#define SPI_MOSI        PINB2
#define SPI_SCLK        PINB1

static FILE mystdout = {0};


void SPI_MasterInit(void) {
    // outputs, also for the /SS pin, to stop it from flicking to slave
    SPI_DDR |= _BV(SPI_MOSI) | _BV(SPI_SCLK) | _BV(MRF_PIN_CS);
    /* Enable SPI, Master, set clock rate fck/4 */
    SPCR = (1 << SPE) | (1 << MSTR);
    // So, that's either 4Mhz, or 2Mhz, depending on whether Fosc is
    // 16mhz crystal, or 8mhz clock prescaled?
    // If I can read the mrf24j sheet properly, this should work right
    // up to 10Mhz, but let's not push it.... (yet)
}

static int uart_putchar(char c, FILE* stream) {
    return usb_debug_putchar(c);
}

void init(void) {
    clock_prescale_set(clock_div_2); // we run at 3.3v
    fdev_setup_stream(&mystdout, uart_putchar, NULL, _FDEV_SETUP_WRITE);
    stdout = &mystdout;
    usb_init();
    MRF_CONFIG;
    SPI_MasterInit();

    // interrupt pin from mrf
    EIMSK |= (1<<INT0);

    while (!usb_configured()) {
    }
    // Advised to wait a bit longer by pjrc.com
    // as not all drivers will be ready even after host is up
    _delay_ms(500);
}


ISR(INT0_vect) {
    mrf_interrupt_handler();
}

void handle_rx(mrf_rx_info_t *rxinfo, uint8_t *rx_buffer) {
    printf_P(PSTR("Received a packet: %d bytes long\n"), rxinfo->frame_length);
    printf_P(PSTR("Packet data:\n"));
    for (int i = 0; i <= rxinfo->frame_length; i++) {
        printf("%x", rx_buffer[i]);
    }
    printf_P(PSTR("\nLQI/RSSI=%d/%d\n"), rxinfo->lqi, rxinfo->rssi);
}

void handle_tx(mrf_tx_info_t *txinfo) {
    if (txinfo->tx_ok) {
        printf_P(PSTR("TX went ok, got ack\n"));
    } else {
        printf_P(PSTR("TX failed after %d retries\n"), txinfo->retries);
    }
}

int main(void) {
    init();

    printf_P(PSTR("woke up...woo\n"));
    mrf_reset(&MRF_PORT, MRF_PIN_RESET);
    mrf_init(&MRF_PORT, MRF_PIN_CS);

    mrf_pan_write(0xcafe);
    mrf_address16_write(0x6001);
    //mrf_write_short(MRF_RXMCR, 0x01); // promiscuous!
    sei();
    uint32_t roughness = 0;
    while (1) {
        roughness++;
        mrf_check_flags(&handle_rx, &handle_tx);
        // about a second or so...
        if (roughness > 0x00050000) {
            printf_P(PSTR("txxxing...\n"));
            mrf_send16(0x4202, 4, "abcd");
            roughness = 0;
        }
    }
}

