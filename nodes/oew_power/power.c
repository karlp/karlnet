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


/// All this shit is global because I hate embedded worlds and compilers :(
//Sample variables
int lastSampleV, lastSampleI, sampleV, sampleI;

//Filter variables
double lastFilteredV, lastFilteredI, filteredV, filteredI;
double filterTemp;

//Stores the phase calibrated instantaneous voltage.
//double shiftedV;

int adc_read(unsigned char muxbits) {
    ADMUX = VREF_AVREF | (muxbits);
    ADCSRA |= (1 << ADSC); // begin the conversion
    while (ADCSRA & (1 << ADSC)); // wait for the conversion to complete

    // toss the first result and do it again...
    ADCSRA |= (1 << ADSC); // begin the conversion
    while (ADCSRA & (1 << ADSC)); // wait for the conversion to complete

    uint8_t lsb = ADCL; // read the LSB first
    return (ADCH << 8) | lsb; // read the MSB and return 10 bit result
}

/**
 * These two really need to go to some common code
 */
void uart_print_short(unsigned int val) {
    uart_putc((unsigned char) (val >> 8));
    uart_putc((unsigned char) (val & 0xFF));
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

int dostuff(kpacket* packetp) {
    int numberOfSamples = 3000;

/*
    int lastSampleV = 0;
    int lastSampleI = 0;
    int sampleV = 0;
    int sampleI = 0;
    //Filter variables
    double lastFilteredV = 0;
    double lastFilteredI = 0;
    double filteredV = 0;
    double filteredI = 0;
*/
    //Stores the phase calibrated instantaneous voltage.
    double shiftedV = 0;

    //Power calculation variables
    double sqI = 0;
    double sqV = 0;
    double instP = 0;
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
        //1) square voltage values
        sqV = filteredV * filteredV;
        //2) sum
        sumV += sqV;

        //Root-mean-square method current
        //1) square current values
        sqI = filteredI * filteredI;
        //2) sum
        sumI += sqI;

        //Instantaneous Power
        instP = shiftedV * filteredI;
        //Sum
        sumP += instP;
    }

    //Calculation of the root of the mean of the voltage and current squared (rms)
    //Calibration coeficients applied.
    Vrms = V_RATIO * sqrt(sumV / numberOfSamples);
    Irms = I_RATIO * sqrt(sumI / numberOfSamples);

    //Calculation power values
    realPower = V_RATIO * I_RATIO * sumP / numberOfSamples;
    apparentPower = Vrms * Irms;
    powerFactor = realPower / apparentPower;

    //Reset accumulators

    sumV = 0;
    sumI = 0;
    sumP = 0;


    ksensor rp = {1, (uint32_t) (realPower * 100)};
    ksensor pf = {2, (uint32_t) (powerFactor * 1000)};
    ksensor vrms = {3, (uint32_t) (Vrms * 100)};

    packetp->ksensors[0] = rp;
    packetp->ksensors[1] = pf;
    packetp->ksensors[2] = vrms;


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
        dostuff(packetp);
        xbee_send_16(1, packet);
    }
}


