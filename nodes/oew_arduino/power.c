// Karl Palsson, Oct 2010
// based on: http://openenergymonitor.org/emon/node/58

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "uart.h"
#include "karlnet.h"
#include "xbee-api.h"



#define ADC_ENABLE (ADCSRA |= (1<<ADEN))
#define ADC_DISABLE  (ADCSRA &= ~(1<<ADEN))

#define ADC_PIN_CURRENT (1<<MUX0)
#define ADC_PIN_VOLTAGE (1<<MUX1)

#define BAUD_RATE 19200
#define VREF_VCC  (0)
#define VREF_AVREF  (1 << REFS0)

// Magic config values derived with the arduino code for my hardware...
#define VCAL 0.95
#define ICAL 1.90

#define AC_WALL_VOLTAGE 225
#define AC_ADAPTER_VOLTAGE 9.8
#define AC_VOLTAGE_DIV_RATIO 6.7
#define AC_ADAPTER_RATIO (AC_WALL_VOLTAGE / AC_ADAPTER_VOLTAGE)
#define CT_CONSTANT 16.216216

#define V_RATIO ((AC_ADAPTER_RATIO * AC_VOLTAGE_DIV_RATIO * 5) / 1024 * VCAL)
#define I_RATIO ((CT_CONSTANT) * 5 / 1024 * ICAL)
#define PHASECAL 2.4

unsigned int adc_read(unsigned char muxbits)
{
    ADMUX = VREF_AVREF | (muxbits);
    ADCSRA |= (1<<ADSC);               // begin the conversion
    while (ADCSRA & (1<<ADSC)) ;     // wait for the conversion to complete
    unsigned char lsb = ADCL;       // read the LSB first
    return (ADCH << 8) | lsb;          // read the MSB and return 10 bit result
}


/**
 * These two really need to go to some common code
 */
void uart_print_short(unsigned int val) {
        uart_putc((unsigned char)(val >> 8));
        uart_putc((unsigned char)(val & 0xFF));
}


void init(void) {
    clock_prescale_set(0);
    uart_init(UART_BAUD_SELECT(BAUD_RATE,F_CPU));
    // prescale down to 125khz for accuracy
    ADCSRA = (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);  // enable ADC
    
    power_adc_enable();
    ADC_ENABLE;
    // normally, lots of low power stuff...
}

// Potentially, some of this could be calculated pc side, and we just return
// the sums and the counts, but that's harder to filter enroute
int meterPower(double *realPowerIn, double *powerFactorIn, double *VrmsIn) {

    int numSamples = 3000;

    int lastSampleV, lastSampleI, sampleV, sampleI;
    double lastFilteredV, lastFilteredI, filteredV, filteredI;
    double instP, sumI, sumV, sumP;

    for (int i = 0; i < numSamples; i++) {
        lastSampleV = sampleV;
        lastSampleI = sampleI;
        
        sampleV = adc_read(ADC_PIN_VOLTAGE);
        sampleI = adc_read(ADC_PIN_CURRENT);

        lastFilteredV = filteredV;
        lastFilteredI = filteredI;
        
        // digital high pass filter to remove 2.5V DC offset...
        filteredV = 0.996 * (lastFilteredV + sampleV - lastSampleV);
        filteredI = 0.996 * (lastFilteredI + sampleI - lastSampleI);

        double shiftedV = lastFilteredV + PHASECAL * (filteredV - lastFilteredV);

        // RMS voltage
        sumV += (filteredV * filteredV);
        sumI += (filteredI * filteredI);

        instP = shiftedV * filteredI;
        sumP += instP;
    }
    
    // Finish rms calculations
    double Vrms;
    Vrms = (V_RATIO) * sqrt(sumV / numSamples);
    double Irms;
    Irms = (I_RATIO) * sqrt(sumI / numSamples);

    double realPower = V_RATIO * I_RATIO * sumP / numSamples;
    double apparentPower = Vrms * Irms;
    double powerFactor = realPower / apparentPower;

    *realPowerIn = realPower;
    *powerFactorIn = powerFactor;
    *VrmsIn = Vrms;
    return 0;
}

    
        


int main(void) {
    init();
    kpacket packet;
    packet.header = 'x';
    packet.version = 1;
    packet.nsensors = 3;
    sei();
    
    while (1) {
        double realPower;
        double powerFactor;
        double Vrms;

        meterPower(&realPower, &powerFactor, &Vrms);

        ksensor rp = { 1, (uint32_t) (realPower * 100) };
        ksensor pf = { 2, (uint32_t) (powerFactor * 1000) };
        ksensor vrms = {3, (uint32_t) (Vrms * 100) };

        packet.ksensors[0] = rp;
        packet.ksensors[1] = pf;
        packet.ksensors[2] = vrms;

        xbee_send_16(1, packet);

        _delay_ms(1000);
        
    }
}

        
