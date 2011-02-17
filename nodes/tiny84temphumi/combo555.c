// Karl Palsson, 2010
// 
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h> 
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "suart.h"
#include "karlnet.h"
#include "xbee-api.h"

#include "FreqCounter.h"

#define XBEE_OFF        (PORTA |= (1<<PINA7))
#define XBEE_ON         (PORTA &= ~(1<<PINA7))
#define XBEE_CONFIG     (DDRA |= (1<<PINA7))

#define ADC_ENABLE  (ADCSRA |= (1<<ADEN))
#define ADC_DISABLE  (ADCSRA &= ~(1<<ADEN))

#define PROBE_CHANNEL1_ENABLED_PIN      PINB0
#define PROBE_CHANNEL2_ENABLED_PIN      PINB1

#define BAUD_RATE 19200


#define VREF_VCC  0
#define VREF_11  (1<<REFS1) | (1<<REFS0) // requires external cap at aref!

void init_adc_regular(uint8_t muxBits) {
    ADMUX = VREF_VCC | muxBits;
}

void init_adc_int_temp(void)
{
    ADMUX = VREF_11 | (1<<MUX5) | (1<<MUX1); // internal temp sensor
}

unsigned int adc_read(void)
{
    ADCSRA |= (1<<ADSC);               // begin the conversion
    while (ADCSRA & (1<<ADSC)) ;   // wait for the conversion to complete
    unsigned char lsb = ADCL;                              // read the LSB first
    return (ADCH << 8) | lsb;          // read the MSB and return 10 bit result
}

void init(void) {
	XBEE_CONFIG;
        XBEE_OFF;
        clock_prescale_set(0);
        ADCSRA = (1<<ADPS2) | (1<<ADPS1); // 8Mhz / 64 = 125khz => good

        TX_CONFIG;

        // turn off things we don't ever need...

        // turn off analog comparator
        ACSR &= ~(1<<ACIE);
        ACSR |= (1<<ACD);

        // disable all digital input buffers, we only use analog inputs...
        //DIDR0 = 0xff; lies, damn lies! T1 is on an ADC channel!
        
/*
        power_timer0_disable();
        power_spi_disable();
        power_twi_disable();
*/

        set_sleep_mode(SLEEP_MODE_PWR_DOWN);  // deeeeeep sleep

        MCUSR &= ~(1<<WDRF);
        // begin timed sequence
        WDTCSR |= (1<<WDCE) | (1<<WDE);
        // set new watchdog timeout value
        WDTCSR = (1<<WDP3) | (1<<WDIE);  // 4 secs

}

EMPTY_INTERRUPT(WDT_vect);

int main(void) {
	init();
        kpacket packet;
        packet.header = 'x';
        packet.version = 1;
        packet.nsensors = 3;

        sei();
        unsigned int sensor1;
        uint32_t sensor2;
        ksensor s1 = { 0, 0 };
        ksensor s2 = { 0, 0 };

        while (1) {
                FreqCounter__start(1, 1000); // start, gate time 1000ms
                power_adc_enable();
                _delay_us(70);  // bandgap wakeup time from datasheet (worst case)
                ADC_ENABLE;
                // Switches are normally closed
                if (!(PINB & (1<<PROBE_CHANNEL1_ENABLED_PIN))) {
                    init_adc_regular(1);
                    adc_read();  // toss first result
                    sensor1 = adc_read();
                    s1.type = NTC_10K_3V3;
                    s1.value = sensor1;
                } else {
                    s1.type = SENSOR_TEST;
                    s1.value = 0xbabe;
                }

                s2.type = SENSOR_TEST;
                s2.value = 0xface;
                init_adc_int_temp();
                adc_read();
                unsigned int internalTemp = adc_read();
                ADC_DISABLE;
                power_adc_disable();

                ksensor s3 = { TEMP_INT_VREF11, internalTemp };


                while (f_ready == 0) {
                    ;
                }          // wait until counter ready

                uint32_t myfreq = f_freq;
                s2.type = FREQ_1SEC;
                s2.value = myfreq;


                packet.ksensors[0] = s1;
                packet.ksensors[1] = s2;
                packet.ksensors[2] = s3;

		XBEE_ON;
		_delay_ms(2); // xbee manual says 2ms for sleep mode 2, 13 for sm1
                xbee_send_16(1, packet);
                //xbee_send_16(0x4202, packet);
		XBEE_OFF;

		// now sleep!
                _delay_ms(3000);
/*
		sei();
		sleep_mode();
		sleep_disable();
		cli();
*/
	}
}

