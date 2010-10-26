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

#define ADC_PIN_CURRENT (0x01)
#define ADC_PIN_VOLTAGE (0x02)

#define BAUD_RATE 19200
#define VREF_VCC  (0)
#define VREF_AVREF  (1 << REFS0)

// Voltage is reduced both by wall xfmr & voltage divider
#define AC_WALL_VOLTAGE        225
#define AC_ADAPTER_VOLTAGE     9.8
// Ratio of the voltage divider in the circuit
#define AC_VOLTAGE_DIV_RATIO   6.7
// CT: Voltage depends on current, burden resistor, and turns
#define CT_BURDEN_RESISTOR    37
#define CT_TURNS              600

//Calibration coeficients
//These need to be set in order to obtain accurate results
//Set the above values first and then calibrate futher using normal calibration method described on how to build it page.
#define VCAL (0.95)
#define ICAL (1.90)
double PHASECAL = 2.4;

// Initial gueses for ratios, modified by VCAL/ICAL tweaks
#define AC_ADAPTER_RATIO       (AC_WALL_VOLTAGE / AC_ADAPTER_VOLTAGE)
double V_RATIO = AC_ADAPTER_RATIO * AC_VOLTAGE_DIV_RATIO * 5 / 1024 * VCAL;
double I_RATIO = (long double) CT_TURNS / CT_BURDEN_RESISTOR * 5 / 1024 * ICAL;


// These need to be global, otherwise you have a startup inaccuracy on every reading.
// the sliding averages needs the "prior" values, and if you reset them to zero each time,
// you're screwed.
//Sample variables
int lastSampleV, lastSampleI, sampleV, sampleI;

//Filter variables
double lastFilteredV, lastFilteredI, filteredV, filteredI;

int adc_read(unsigned char muxbits) {
    ADMUX = VREF_AVREF | (muxbits);
    ADCSRA |= (1 << ADSC); // begin the conversion
    while (ADCSRA & (1 << ADSC)); // wait for the conversion to complete

    uint8_t lsb = ADCL; // read the LSB first
    return (ADCH << 8) | lsb; // read the MSB and return 10 bit result
}


void init(void) {
    clock_prescale_set(0);
    uart_init(UART_BAUD_SELECT(BAUD_RATE, F_CPU));
    // prescale down to 125khz for accuracy
    ADCSRA = (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // enable ADC

    power_adc_enable();
    ADC_ENABLE;
    // normally, lots of low power stuff...
}

int meter_power(kpacket* packetp) {
    int numberOfSamples = 5000;

    //Stores the phase calibrated instantaneous voltage.
    double shiftedV = 0;

    //Power calculation variables
    double sumI = 0;
    double sumV = 0;
    double sumP = 0;

    //Useful value variables
    double realPower,
            apparentPower,
            powerFactor,
            Vrms,
            Irms;

    for (int n = 0; n < numberOfSamples; n++) {

        //Used for offset removal
        lastSampleV = sampleV;
        lastSampleI = sampleI;

        //Read in voltage and current samples.
        sampleV = adc_read(ADC_PIN_VOLTAGE);
        sampleI = adc_read(ADC_PIN_CURRENT);

        //Used for offset removal
        lastFilteredV = filteredV;
        lastFilteredI = filteredI;

        //Digital high pass filters to remove 2.5V DC offset.
        filteredV = 0.996 * (lastFilteredV + sampleV - lastSampleV);
        filteredI = 0.996 * (lastFilteredI + sampleI - lastSampleI);

        //Phase calibration goes here.
        shiftedV = lastFilteredV + PHASECAL * (filteredV - lastFilteredV);

        //Root-mean-square method voltage
        sumV += (filteredV * filteredV);

        //Root-mean-square method current
        sumI += (filteredI * filteredI);

        //Instantaneous Power
        sumP += (shiftedV * filteredI);
    }

    //Calculation of the root of the mean of the voltage and current squared (rms)
    //Calibration coeficients applied.
    Vrms = V_RATIO * sqrt(sumV / numberOfSamples);
    Irms = I_RATIO * sqrt(sumI / numberOfSamples);

    //Calculation power values
    realPower = V_RATIO * I_RATIO * sumP / numberOfSamples;
    apparentPower = Vrms * Irms;
    powerFactor = realPower / apparentPower;


    ksensor rp = {1, (uint32_t) (realPower * 100)};
    ksensor pf = {2, (uint32_t) (powerFactor * 1000)};
    ksensor vrms = {3, (uint32_t) (Vrms * 100)};

    packetp->ksensors[0] = rp;
    packetp->ksensors[1] = pf;
    packetp->ksensors[2] = vrms;

    return 0;
}

int main(void) {
    init();
    kpacket packet;
    kpacket* packetp = &packet;
    packet.header = 'x';
    packet.version = 1;
    packet.nsensors = 3;
    sei();

    while (1) {
        meter_power(packetp);
        xbee_send_16(1, packet);
    }
}


