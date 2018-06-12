#include <ExtendedADCShield.h>
#include <SPI.h>
#include <analogShield.h>   //Include to use analog shield.
#include <math.h>   //avr math library
#include <DueTimer.h> //Hardcoded interrupt timer.

// Initialize the ADC Shield with default pins and 16-bit ADC (LTC1859)
ExtendedADCShield extendedADCShield(16);

unsigned long loopTime = 4000; // microseconds. This variable sets the loop's execution rate.
double adcReadings;
double AnalogOutVal;
int AnalogDACSetting;
unsigned long acquisitionTime;
unsigned long acquisitionTimeMicros;
char SerialChar;
int loopCounter;
int loopExecutionTime, maxLoopExecutionTime, minLoopExecutionTime;
// Filter holders
double x0,x1,x2,x3,Y0,Y1,Y2,Y3;
double freq=100; //Drive frequency in Hz
const double pi=3.14159265359;
void setup() {
  Serial.begin(115200);
  // SPI.begin must be called here on Due only
  SPI.begin();
  
  readADCs();

  // Flush out serial buffer
  while(Serial.available() > 0){
    SerialChar = Serial.read();
  }

  // Absolute timer interrupt initialization
  Timer3.attachInterrupt(OneCycle);
  Timer3.start(loopTime);

}

void loop() {
  while(1){
     // Parse serial inputs
    while(Serial.available() > 0){
      parseSerialArguments();
    }       
  }
}
// Main loop
void OneCycle() {
  // Acquire Signals
  readADCs();
  // Record time 
  timestamp(); 
  // Apply AA filter  
  filter();

  // Output to COM channel
  Serial.print(acquisitionTimeMicros);
  Serial.print("\t");
  Serial.print(adcReadings);
  Serial.print("\r");
  Serial.print("\n");

  // Calibration line
  AnalogOutVal=3*sin(2*pi*freq*micros()*pow(10,-6));
  
  analogOutput();  
}
void filter(){
  x0=adcReadings;
  // Third order butterworth at sampFreq/4
  Y0=-0.3333*Y1+0.1667*x0+0.5*x1+0.5*x2+0.1667*x3;
  // Third order butterworth at sampFreq/2
  //Y0=-3*Y1-3*Y2-1*Y3+1*x0+3*x1+3*x2+1*x3;
  
  adcReadings=Y0;
  // Shift values
  Y3=Y2; Y2=Y1; Y1=Y0;
  x3=x2; x2=x1; x1=x0;
}
void timestamp(){
  acquisitionTime = millis();
  acquisitionTimeMicros = micros();
} 
// Function configures and acquires from the 16-bit ADC.
// Function is void in order to avoid memory management.
void readADCs(){
  //Strange modulus is there because this is "read, configNext". Gotta prepare for the next read.
  adcReadings = extendedADCShield.analogReadConfigNext(1, SINGLE_ENDED, BIPOLAR, RANGE5V);

}
void analogOutput(){
  
  // Make sure you aren't creating infinite energy
  if ( isnan(AnalogOutVal) ){
    AnalogOutVal = 0.0;
  }
  if ( isinf(AnalogOutVal) ){
    AnalogOutVal = 0.0;
  }
  // Calculate bit value for DAC (+- 5V)
  AnalogDACSetting = (int) ( AnalogOutVal/10.0 * 32768.0 + 32768);
  // Output the value to the DAC
  analog.write(0, AnalogDACSetting );
  analog.write(1, AnalogDACSetting );
  analog.write(2, AnalogDACSetting );
}

void parseSerialArguments(){
  SerialChar = Serial.read();
}
