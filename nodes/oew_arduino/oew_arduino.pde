//Basic energy monitoring sketch
//Authors: Trystan Lea, Eric Sandeen

//Licenced under GNU General Public Licence more details here
// openenergymonitor.org

//Sketch measures voltage and current. 
//and then calculates useful values like real power,
//apparent power, powerfactor, Vrms, Irms.

//Setup variables
int numberOfSamples = 3000;

//Set Voltage and current input pins
int inPinV = 2;
int inPinI = 1;

//Sanity check calibration method thanks to Eric Sandeen http://sandeen.net/wordpress
//See discussion here: http://openenergymonitor.org/emon/node/59
//Enter the values of your setup below 

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
double VCAL = 0.95;
double ICAL = 1.90;
double PHASECAL = 2.4;

// Initial gueses for ratios, modified by VCAL/ICAL tweaks
#define AC_ADAPTER_RATIO       (AC_WALL_VOLTAGE / AC_ADAPTER_VOLTAGE)
double V_RATIO = AC_ADAPTER_RATIO * AC_VOLTAGE_DIV_RATIO * 5 / 1024 * VCAL;
double I_RATIO = (long double)CT_TURNS / CT_BURDEN_RESISTOR * 5 / 1024 * ICAL;

//Sample variables
int lastSampleV,lastSampleI,sampleV,sampleI;

//Filter variables
double lastFilteredV, lastFilteredI, filteredV, filteredI;
double filterTemp;

//Stores the phase calibrated instantaneous voltage.
double shiftedV;

//Power calculation variables
double sqI,sqV,instP,sumI,sumV,sumP;

//Useful value variables
double realPower,
       apparentPower,
       powerFactor,
       Vrms,
       Irms;
       
void setup()
{
   Serial.begin(9600); 
}

void loop()
{ 

for (int n=0; n<numberOfSamples; n++)
{

   //Used for offset removal
   lastSampleV=sampleV;
   lastSampleI=sampleI;
   
   //Read in voltage and current samples.   
   sampleV = analogRead(inPinV);
   sampleI = analogRead(inPinI);
   
   //Used for offset removal
   lastFilteredV = filteredV;
   lastFilteredI = filteredI;
  
   //Digital high pass filters to remove 2.5V DC offset.
   filteredV = 0.996*(lastFilteredV+sampleV-lastSampleV);
   filteredI = 0.996*(lastFilteredI+sampleI-lastSampleI);
   
   //Phase calibration goes here.
   shiftedV = lastFilteredV + PHASECAL * (filteredV - lastFilteredV);
  
   //Root-mean-square method voltage
   //1) square voltage values
   sqV= filteredV * filteredV;
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
   sumP +=instP;
}

//Calculation of the root of the mean of the voltage and current squared (rms)
//Calibration coeficients applied. 
Vrms = V_RATIO*sqrt(sumV / numberOfSamples); 
Irms = I_RATIO*sqrt(sumI / numberOfSamples); 

//Calculation power values
realPower = V_RATIO*I_RATIO*sumP / numberOfSamples;
apparentPower = Vrms * Irms;
powerFactor = realPower / apparentPower;

//Output to serial
Serial.print(realPower);
Serial.print(' ');
Serial.print(apparentPower);
Serial.print(' ');
Serial.print(powerFactor);
Serial.print(' ');
Serial.print(Vrms);
Serial.print(' ');
Serial.println(Irms);

//Reset accumulators
sumV = 0;
sumI = 0;
sumP = 0;

}

