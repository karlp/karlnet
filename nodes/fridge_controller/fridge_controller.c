// Karl Palsson, 2011
// 
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h> 
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "lib_mrf24j.h"
#include "karlnet.h"

#define TICKS_PER_500MS 15625

// USCK and DO pins are output
#define SPI_CONFIG  (DDRA |= (1<<PINA5) | (1<<PINA4))

#define RELAY_ENABLE (DDRA |= (1<<PINA2))
#define RELAY_OFF (PORTA &= ~(1<<PINA2))
#define RELAY_ON  (PORTA |= (1<<PINA2))

#define MRF_DDR (DDRB)
#define MRF_PORT (PORTB)
#define MRF_PIN_RESET   (PINB1)
#define MRF_PIN_CS      (PINB0)
#define MRF_PIN_INT     (PINB2)

typedef struct _config {
    uint16_t threshold_value;
    uint16_t min_on_time;
    uint16_t min_off_time;
    uint16_t interval_report;
    uint16_t rf_panid;
    uint16_t rf_ourid;
} configt;

configt config_eeprom EEMEM = {
    .threshold_value = 300,
    .min_off_time = 8,
    .min_on_time = 8,
    .interval_report = 1,
    .rf_panid = 0xcafe,
    .rf_ourid = 0x6002
};

uint32_t flags;
uint8_t rx_command_count;
// interval_report 1 ==> 2 secs
// interval report 4 ==> 8 secs

void init_adc(void) {
    // 1.1V aref, adc0.
    ADMUX = (1<<REFS1);
    ADCSRA |= (1<<ADEN);
}

/**
 * Set up a 500ms per tick timer, using Timer 1
 */
void init_slow_timer(void) {
    // 8mhz internal oscillator / 1024 ==> 7812hz
    // plus clear timer on compare match.
    OCR1A = TICKS_PER_500MS;
    TCCR1B = (1<<WGM12) | (1<<CS12) | (1<<CS10);
    //TCNT1 = 0;
    TIMSK1 = (1<<OCIE1A);
}

uint16_t adc_read(void)
{
    ADCSRA |= (1<<ADSC);               // begin the conversion
    while (ADCSRA & (1<<ADSC)) ;   // wait for the conversion to complete
    unsigned char lsb = ADCL;                              // read the LSB first
    return (ADCH << 8) | lsb;          // read the MSB and return 10 bit result
}


void SPI_MasterInit(void) {
}

void init(configt *config) {
    MRF_DDR = (1<<MRF_PIN_RESET) | (1<<MRF_PIN_CS);
    SPI_CONFIG;

    // interrupt pin from mrf, low level triggered
    //EIMSK |= (1<<INT0);
    GIMSK |= (1<<INT0);
    RELAY_ENABLE;
    init_adc();
    init_slow_timer();

    eeprom_read_block(config, &config_eeprom, sizeof(configt));

    mrf_reset(&MRF_PORT, MRF_PIN_RESET);
    mrf_init();
    mrf_pan_write(config->rf_panid);
    mrf_address16_write(config->rf_ourid);
}

volatile uint16_t tick_reporting;
volatile uint32_t tick_motor;

ISR(TIM1_COMPA_vect) {
    tick_reporting++;
    tick_motor++;
    // make sure it's working...
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

inline uint8_t get_relay_state(void) {
    if (PORTA & (1<<PINA2)) {
        return 1;
    }
    return 0;
}

// TODO - FIXME
int handle_network_traffic(configt *config) {

    gotrx = 0;
    cli();
    //print("Received a packet!\n\r");
    rx_command_count++;
    flags = rx_command_count;

    mrf_write_short(MRF_BBREG1, 0x04);  // RXDECINV - disable receiver

    uint8_t frame_length = mrf_read_long(0x300);  // read start of rxfifo for
    //phex(frame_length);
    //print("\r\nPacket data:\r\n");
    int fptr = 0x301;
/*
    uint8_t b1 = mrf_read_long(fptr++);
    uint8_t b2 = mrf_read_long(fptr++);
    if (b1 == 'k' && b2 == 'k') {
        rx_command_count++;
        config->interval_report = mrf_read_long(fptr++);
        flags = (uint32_t)rx_command_count << 24 | (uint32_t)'k' << 16 | config->interval_report;
    }
*/
    // this should basically ignore the rest of the packet...
    // read the rest of the packet!!!
    int endptr = 0x300 + frame_length;
    for (;fptr <= endptr;) {
        uint8_t tmp = mrf_read_long(fptr++);
    }
    //print("\r\nLQI/RSSI=");
    uint8_t lqi = mrf_read_long(0x300 + frame_length + 1);
    uint8_t rssi = mrf_read_long(0x300 + frame_length + 2);
    //phex(lqi);
    //phex(rssi);

    mrf_write_short(MRF_BBREG1, 0x00);  // RXDECINV - enable receiver
    sei();

    return 0;
}

int main(void) {
    configt config;
    init(&config);
    //mrf_write_short(MRF_RXMCR, 0x01); // promiscuous!

    kpacket2 packet;
    packet.header = 'x';
    packet.versionCount = VERSION_COUNT(KPP_VERSION_2, 3);

    sei();
    while (1) {

        // Read temp/pot
        uint16_t currentValue = adc_read();

        // TODO - insert code here to listen to the mrf and update our config!!
        if (gotrx) {
            handle_network_traffic(&config);
        }

        if (tick_reporting >= config.interval_report) {
            tick_reporting = 0;
            ksensor s1 = {TMP36_VREF11, currentValue};
            ksensor s2 = {KPS_RELAY_STATE, get_relay_state()};
            ksensor s3 = {KPS_SENSOR_TEST, flags};
            packet.ksensors[0] = s1;
            packet.ksensors[1] = s2;
            packet.ksensors[2] = s3;
            mrf_send16(0x4202, sizeof(kpacket2), &packet);
        }

        // now, check the threshold.  If we're above (too hot),
        // we can turn the motor on, but only if it's off, and only if
        // it's been longer than min_off_time since it was last on...
        if (currentValue > config.threshold_value) {
            if (!get_relay_state()) {
                if (tick_motor >= config.min_off_time) {
                    RELAY_ON;
                    tick_motor = 0;
                }
            } else {
                // motor is already on, do nothing...
            }
        }
        // likewise, if we're too cold, (anywhere below)
        // we want to turn the motor off, but only if it's on already, and
        // only if it's been on longer than min_on_time
        if (currentValue <= config.threshold_value) {
            if (get_relay_state()) {
                if (tick_motor >= config.min_on_time) {
                    RELAY_OFF;
                    tick_motor = 0;
                }
            } else {
                // motor is already off, do nothing
            }
        }
    }
}

