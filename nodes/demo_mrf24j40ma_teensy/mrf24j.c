// Karl Palsson, 2011
// false.ekta.is
// BSD/MIT Licensed.
// Keep netbeans happy
#ifndef __AVR_ATmega32U4__
#define __AVR_ATmega32U4__
#endif

#include <stdio.h>
#include <avr/io.h> 
#include <avr/pgmspace.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <util/delay.h>
// usb hid...
#include "usb_debug_only.h"
#include "lib_mrf24j.h"

#define MRF_RESET_DDR DDRB
#define MRF_RESET_PORT PORTB
#define MRF_RESET_PIN PINB4
#define MRF_CS_DDR DDRB
#define MRF_CS_PORT PORTB
#define MRF_CS_PIN PINB0

#define MRF_RESET_CONFIG  (MRF_RESET_DDR |= (1<<MRF_RESET_PIN))
#define MRF_CS_CONFIG  (MRF_CS_DDR |= (1<<MRF_CS_PIN))

#define SPI_DDR     DDRB
#define SPI_MISO        PINB3
#define SPI_MOSI        PINB2
#define SPI_SCLK        PINB1

static FILE mystdout = {0};


void SPI_MasterInit(void) {
    // outputs, also for the /SS pin, to stop it from flicking to slave
    SPI_DDR |= _BV(SPI_MOSI) | _BV(SPI_SCLK) | _BV(MRF_CS_PIN);
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
    MRF_RESET_CONFIG;
    MRF_CS_CONFIG;
    SPI_MasterInit();

    // interrupt pin from mrf
    EIMSK |= (1<<INT0);

    while (!usb_configured()) {
    }
    // Advised to wait a bit longer by pjrc.com
    // as not all drivers will be ready even after host is up
    _delay_ms(500);
}

static inline void mrf_interrupt_disable(void) {
    EIMSK &= ~(_BV(INT0));
}
static inline void mrf_interrupt_enable(void) {
    EIMSK |= _BV(INT0);
}

ISR(INT0_vect) {
    mrf_interrupt_handler();
}

void handle_rx(mrf_rx_info_t *rxinfo, uint8_t *rx_buffer) {
    mrf_interrupt_disable();
    printf_P(PSTR("Received a packet: %u bytes long\n"), rxinfo->frame_length);
    printf("headers:");
    switch (rxinfo->frame_type) {
        case MAC_FRAME_TYPE_BEACON: printf("[ft:beacon]"); break;
        case MAC_FRAME_TYPE_DATA: printf("[ft:data]"); break;
        case MAC_FRAME_TYPE_ACK: printf("[ft:ack]"); break;
        case MAC_FRAME_TYPE_MACCMD: printf("[ft:mac command]"); break;
        default: printf("[ft:reserved]");
    }
    if (rxinfo->pan_compression) {
        printf("[pan comp]");
    }
    if (rxinfo->ack_bit) {
        printf("[ack bit]");
    }
    if (rxinfo->security_enabled) {
        printf("[security]");
    }
    switch (rxinfo->dest_addr_mode) {
        case MAC_ADDRESS_MODE_NONE: printf("[dam:nopan,noaddr]"); break;
        case MAC_ADDRESS_MODE_RESERVED: printf("[dam:reserved]"); break;
        case MAC_ADDRESS_MODE_16: printf("[dam:16bit]");break;
        case MAC_ADDRESS_MODE_64: printf("[dam:64bit]");break;
    }
    switch (rxinfo->frame_version) {
        case MAC_FRAME_VERSION_2003: printf("[fv:std2003]");break;
        case MAC_FRAME_VERSION_2006: printf("[fv:std2006]");break;
        default: printf("[fv:future]");break;
    }
    switch (rxinfo->src_addr_mode) {
        case MAC_ADDRESS_MODE_NONE: printf("[sam:nopan,noaddr]"); break;
        case MAC_ADDRESS_MODE_RESERVED: printf("[sam:reserved]"); break;
        case MAC_ADDRESS_MODE_16: printf("[sam:16bit]");break;
        case MAC_ADDRESS_MODE_64: printf("[sam:64bit]");break;
    }

    printf("\nsequence: %d(%x)\n", rxinfo->sequence_number, rxinfo->sequence_number);

    uint8_t i = 0;
    uint16_t dest_pan = 0;
    uint16_t dest_id = 0;
    uint16_t src_pan = 0;
    uint16_t src_id = 0;
    if (rxinfo->dest_addr_mode == MAC_ADDRESS_MODE_16
            && rxinfo->src_addr_mode == MAC_ADDRESS_MODE_16
            && rxinfo->pan_compression) {
        dest_pan = rx_buffer[i++];
        dest_pan |= rx_buffer[i++] << 8;
        dest_id = rx_buffer[i++];
        dest_id |= rx_buffer[i++] << 8;
        src_id = rx_buffer[i++];
        src_id |= rx_buffer[i++] << 8;
        printf("[dpan:%x]", dest_pan);
        printf("[d16:%x]", dest_id);
        printf("[s16:%x]", src_id);
        // TODO Can't move this into the library without doing more decoding in the library...
        i += 2;  // as in the library, module seems to have two useless bytes after headers!
    } else {
        printf("unimplemented address decoding!\n");
    }

    // this will be whatever is still undecoded :)
    printf_P(PSTR("Packet data, starting from %u:\n"), i);
    for (; i < rxinfo->frame_length; i++) {
        printf("%02x,", rx_buffer[i]);
    }
    printf_P(PSTR("\nLQI/RSSI=%d/%d\n"), rxinfo->lqi, rxinfo->rssi);
    mrf_interrupt_enable();
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
    mrf_reset(&MRF_RESET_PORT, MRF_RESET_PIN);
    mrf_init(&MRF_CS_PORT, MRF_CS_PIN);

    mrf_pan_write(0xcafe);
    mrf_address16_write(0x1111);
    //mrf_promiscuous(1);
    sei();
    uint32_t roughness = 0;
    while (1) {
        roughness++;
        mrf_check_flags(&handle_rx, &handle_tx);
        // about a second or so...
        if (roughness > 0x50000) {
            printf_P(PSTR("txxxing...\n"));
            mrf_send16(0x4202, 4, "abcd");
            roughness = 0;
        }
    }
}

