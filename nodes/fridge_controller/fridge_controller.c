// Karl Palsson, 2011
// 
// Keep netbeans happy
#ifndef __AVR_ATmega32U4__
#define __AVR_ATmega32U4__
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h> 
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "lib_mrf24j.h"
#include "karlnet.h"

#define TINY84_HARDWARE
#ifndef TINY84_HARDWARE
#include "pjrc/usb_debug_only.h"
#else
#define printf(x,...)
#endif
#define TICKS_PER_500MS 15625

#if defined(TINY84_HARDWARE)
// USCK and DO pins are output
#define SPI_DDR DDRA
#define SPI_MOSI PINA5
#define SPI_SCLK PINA4
#define RELAY_PORT PORTA
#define RELAY_DDR DDRA
#define RELAY_PIN PINA2
#define MRF_DDR (DDRB)
#define MRF_PORT (PORTB)
#define MRF_PIN_RESET   (PINB1)
#define MRF_PIN_CS      (PINB0)
#define MRF_PIN_INT     (PINB2)
static inline void mrf_enable_interrupts(void) {
    GIMSK |= _BV(INT0);
}
static inline void mrf_disable_interrupts(void) {
    GIMSK &= ~(_BV(INT0));
}

#else
// USCK and DO pins are output
#define SPI_DDR     DDRB
#define SPI_MISO        PINB3
#define SPI_MOSI        PINB2
#define SPI_SCLK        PINB1

#define RELAY_PORT PORTD
#define RELAY_DDR DDRD
#define RELAY_PIN PIND6


#define MRF_DDR DDRB
#define MRF_PORT PORTB
#define MRF_PIN_RESET PINB4
#define MRF_PIN_CS PINB0  // This is also /SS

static inline void mrf_disable_interrupts(void) {
    EIMSK &= ~(_BV(INT0));
}
static inline void mrf_enable_interrupts(void) {
    EIMSK |= _BV(INT0);
}

#endif


#define RELAY_ENABLE (RELAY_DDR |= (1<<RELAY_PIN))
#define RELAY_OFF (RELAY_PORT &= ~(1<<RELAY_PIN))
#define RELAY_ON  (RELAY_PORT |= (1<<RELAY_PIN))



typedef struct _config {
    uint16_t threshold_value;
    uint16_t min_on_time;
    uint16_t min_off_time;
    uint16_t interval_report;
    uint16_t rf_panid;
    uint16_t rf_ourid;
    uint16_t rf_collector;
} configt;

configt config_eeprom EEMEM = {
    .threshold_value = 3,
    .min_off_time = 150,  // about 5 minutes
    .min_on_time = 150,  // about 5 minutes
    .interval_report = 5,
    .rf_panid = 0xcafe,
    .rf_ourid = 0x6002,
    .rf_collector = 0x0001  // teensy receiver station
};

configt config;
uint32_t flags;
// interval_report 1 ==> 2 secs
// interval report 4 ==> 8 secs


// http://www.mail-archive.com/avr-gcc-list@nongnu.org/msg03104.html
static inline uint32_t bswap_32_inline(uint32_t x)
{
        union {
                uint32_t x;
                struct {
                        uint8_t a;
                        uint8_t b;
                        uint8_t c;
                        uint8_t d;
                } s;
        } in, out;
        in.x = x;
        out.s.a = in.s.d;
        out.s.b = in.s.c;
        out.s.c = in.s.b;
        out.s.d = in.s.a;
        return out.x;
}


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

float convert_raw(uint16_t raw) {
    //int32_t milliVolts = ((uint32_t) raw) / 1024 * 1100;
    int32_t milliVolts = ((uint32_t) raw) * 1100;
    milliVolts /= 1024;
    float lessOffset = (float)(milliVolts - 750) / 10.0;
    float tempC = 25 + lessOffset;
    return tempC;
}

static void init_spi(void) {
#if defined (SPCR)
    /* Enable SPI, Master, set clock rate fck/4 */
    SPCR = (1 << SPE) | (1 << MSTR);
#endif
    SPI_DDR |= (_BV(SPI_MOSI) | _BV(SPI_SCLK));
}

#ifndef TINY84_HARDWARE
static int uart_putchar(char c, FILE* stream) {
    return usb_debug_putchar(c);
}
static FILE mystdout = {0};
#endif

void init(configt *config) {
#ifndef TINY84_HARDWARE
    clock_prescale_set(clock_div_2); // we run at 3.3v
    fdev_setup_stream(&mystdout, uart_putchar, NULL, _FDEV_SETUP_WRITE);
    stdout = &mystdout;
    usb_init();
#endif

    MRF_DDR |= (1<<MRF_PIN_RESET) | (1<<MRF_PIN_CS);
    init_spi();
    mrf_enable_interrupts();
    RELAY_ENABLE;
    init_adc();
    init_slow_timer();

    eeprom_read_block(config, &config_eeprom, sizeof(configt));

    mrf_reset(&MRF_PORT, MRF_PIN_RESET);
    mrf_init(&MRF_PORT, MRF_PIN_CS);
    mrf_pan_write(config->rf_panid);
    mrf_address16_write(config->rf_ourid);
#ifndef TINY84_HARDWARE
    while (!usb_configured()) {
    }
    _delay_ms(500);
#endif
    printf("We're awake!\n");
}

volatile uint16_t tick_reporting;
volatile uint32_t tick_motor;

#if defined TINY84_HARDWARE
ISR(TIM1_COMPA_vect) {
#else
ISR(TIMER1_COMPA_vect) {
    putchar('!');
#endif
    tick_reporting++;
    tick_motor++;
    // make sure it's working...
}

ISR(INT0_vect) {
    mrf_interrupt_handler();
}

inline uint8_t get_relay_state(void) {
    if (RELAY_PORT & (_BV(RELAY_PIN))) {
        return 1;
    }
    return 0;
}

void handle_rx(mrf_rx_info_t *rxinfo, uint8_t *rbuf) {
    // we can assume it's for us, because we're in non-prom
    // but we still need to decode the headers, before we get to the kpacket...
    // or... be sneaky...
    if (rxinfo->frame_type != MAC_FRAME_TYPE_DATA) {
        printf("Ignorining non data frame\n");
        return;
    }
    uint8_t  i = 0;
    uint16_t dest_pan = 0;
    uint16_t dest_id = 0;
    uint16_t src_id = 0;
    if (rxinfo->dest_addr_mode == MAC_ADDRESS_MODE_16
            && rxinfo->src_addr_mode == MAC_ADDRESS_MODE_16
            && rxinfo->pan_compression) {
        dest_pan = rbuf[i++];
        dest_pan |= rbuf[i++] << 8;
        dest_id = rbuf[i++];
        dest_id |= rbuf[i++] << 8;
        src_id = rbuf[i++];
        src_id |= rbuf[i++] << 8;
#if PAD_DIGI_HEADER
        // TODO Can't move this into the library without doing more decoding in the library...
        i += 2;  // as in the library, module seems to have two useless bytes after headers!
#endif
    }
    if (dest_id != config.rf_ourid) {
        // unexpected, shouldn't ever happen
        printf("ignoring packet actually meant for %x\n", dest_id);
        return;
    }
    if (src_id != config.rf_collector) {
        // Someone else is trying to tell us to do something!
        printf("Ignoring orders from %x\n", src_id);
        return;
    }

    kpacket2 rxkp;
    memcpy(&rxkp, rbuf+i, sizeof(rxkp));
    if (rxkp.header != 'x') {
        printf("Dropping non kpacket\n");
        return;
    }
    uint8_t version = rxkp.versionCount >> 4;
    uint8_t count = rxkp.versionCount & 0x0f;
    printf("\nRX Kpacket(v:%u, count:%u){\n", version, count);
    for (int k = 0; k < count; k++) {
        rxkp.ksensors[k].value = bswap_32_inline(rxkp.ksensors[k].value);
        printf("\tSensor[t:%x(%c), v=%lx]\n", rxkp.ksensors[k].type,
                rxkp.ksensors[k].type, rxkp.ksensors[k].value);
    }
    uint8_t changed = 0;
    if (rxkp.ksensors[0].type == KPS_COMMAND && rxkp.ksensors[1].type == KPS_COMMAND_ARG) {
        switch (rxkp.ksensors[0].value) {
            case KPS_COMMAND_REPORTING_INTERVAL:
                if (config.interval_report != rxkp.ksensors[1].value) {
                    printf("Updating reporting interval from %u to %lu and saving to eeprom\n",
                            config.interval_report, rxkp.ksensors[1].value);
                    config.interval_report = rxkp.ksensors[1].value;
                    changed = 1;
                }
                break;
            case KPS_COMMAND_THRESHOLD_TEMP:
                if (config.threshold_value != rxkp.ksensors[1].value) {
                    printf("Updating reporting temp threshold from %u to %lu and saving to eeprom\n",
                            config.threshold_value, rxkp.ksensors[1].value);
                    config.threshold_value = rxkp.ksensors[1].value;
                    changed = 1;
                }
                break;
            case KPS_COMMAND_MIN_ON_TIME:
                if (config.min_on_time != rxkp.ksensors[1].value) {
                    printf("Updating min on time from %u to %lu and saving to eeprom\n",
                            config.min_on_time, rxkp.ksensors[1].value);
                    config.min_on_time = rxkp.ksensors[1].value;
                    changed = 1;
                }
                break;
            case KPS_COMMAND_MIN_OFF_TIME:
                if (config.min_off_time != rxkp.ksensors[1].value) {
                    printf("Updating min off time from %u to %lu and saving to eeprom\n",
                            config.min_off_time, rxkp.ksensors[1].value);
                    config.min_off_time = rxkp.ksensors[1].value;
                    changed = 1;
                }
                break;
            case KPS_COMMAND_SET_COLLECTOR:
                if (config.rf_collector != rxkp.ksensors[1].value) {
                    printf("Updating rfcollector from %x to %lx and saving to eeprom\n",
                            config.rf_collector, rxkp.ksensors[1].value);
                    config.rf_collector = rxkp.ksensors[1].value;
                    changed = 1;
                }
                break;
        }
    } else {
        printf("No command seen\n");
    }

    if (changed) {
        eeprom_write_block(&config, &config_eeprom, sizeof(configt));
    }

}

int main(void) {
    init(&config);

    kpacket2 packet;
    packet.header = 'x';
    packet.versionCount = VERSION_COUNT(KPP_VERSION_2, 3);

    sei();
    while (1) {

        mrf_check_flags(&handle_rx, NULL);

        // Read temp
        uint32_t currentValue = adc_read();
        currentValue += adc_read();
        currentValue += adc_read();
        currentValue += adc_read();
        currentValue /= 4;
        float temp = convert_raw(currentValue);

        if (tick_reporting >= config.interval_report) {
            printf("config.interval = %u\n", config.interval_report);
            tick_reporting = 0;
            ksensor s1 = {TMP36_VREF11, currentValue};
            ksensor s2 = {KPS_RELAY_STATE, get_relay_state()};
            ksensor s3 = {KPS_SENSOR_TEST, temp * 100};
            packet.ksensors[0] = s1;
            packet.ksensors[1] = s2;
            packet.ksensors[2] = s3;
            printf("txing...\n");
            mrf_send16(config.rf_collector, sizeof(kpacket2), (char*)&packet);
        }

        // now, check the threshold.  If we're above (too hot),
        // we can turn the motor on, but only if it's off, and only if
        // it's been longer than min_off_time since it was last on...
        if (temp > config.threshold_value) {
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
        if (temp <= config.threshold_value) {
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

