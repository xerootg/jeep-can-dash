#include <SPI.h>
#include "mcp_can.h"

/*
wishlist:
TPMS - https://github.com/jboone/tpms
Gas Mileage
*/

//non-boolean values
int backlightIntensity = 0;
int rpm = 0;
unsigned long odometer = 0;
float speed = 0.0;
float oilPressure = 0.0;
float coolantTemp = 0.0;
unsigned int fuelLevel = 0;
float batteryVoltage = 0.0;
// here are our boolean types
boolean isRunning = false;
boolean isBacklightOn = false; //this is the bezel's backlighting - usually set when
boolean isLowWasher = false;
boolean isSecurity = false;
boolean isCruiseActive = false;
boolean isHatchAjar = false;
boolean isEBrakeSet = false;
boolean isABSError = false;
boolean isTurnRight = false;
boolean isTurnLeft = false;
boolean isHighBeam = false;
boolean isSeatBelt = false;
boolean isCELSet = false;
boolean isCheckGaugeSet = false;
boolean isAirbagError = false;
boolean isFuelLevelLow = false;
boolean isFulltime4wd = false;
boolean isParttime4wd = false;
//status variables
int checkGaugeCount = 0;
float oilPressureLow = 15.0;
boolean oilPressureFault = false;
float coolantTempHigh = 220.0;
boolean coolantTempFault = false;
unsigned int fuelLevelLow = 10;
//boolean fuelLevelFault = false; ---- this is a global above, with a getter and setter
float batteryVoltageLow = 12.0;
float batteryVoltageHigh = 16.0;
boolean batteryVoltageFault = false;


// ------------- // Jeep specific

// Constants for math and conversions
const int rpmPerQuad = 2700;
const int rpmOffset = 350 ;

// Constants for pin definition
const int melexisSelectPin1 = 9;
const int selectPinMISO=7;
const int selectPinMOSI=11;
const int clockPin=13;
const int ledPin=5;
const int resetPin=7;
// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
const int SPI_CS_PIN = 10;

// Variables for math and conversion
byte testValueA=0b10010000;
byte testValueB=0b00000001;
byte maskSpeedo=0b10010000;
byte maskTach=0b10110000;
byte maskValueA=0b00001111;
byte maskValueB=0b11111100;
byte value=0;
byte quadSpeedo= 0b00000000;
byte quadTach= 0b00000000;
word wordTach = 0;
int speedo=500;
int increment = 1;
byte rpm_byte = 30; // tach in binary, 64.5 rpm/bit
int rpm_int = 1000; // tach in revolutions per minute
unsigned char len = 0;
unsigned char buf[8] = {65, 0, 0, 0, 0, 0, 30, 0};
unsigned int canId = 640;

boolean can_init = false;

MCP_CAN CAN(SPI_CS_PIN); // Set CS pin for CAN shield

// here are our non-boolean value getters and putters
int getBacklightIntensity(){
  return backlightIntensity;
}
void putBacklightIntensity(int currentBLI){
  backlightIntensity = currentBLI;
  if (currentBLI>0){
  putIsBacklightOn(true);
  }else{
    putIsBacklightOn(false);
  }
  
}
int getRPM(){
  return rpm;
}
void putRPM(int currentRpm){
  rpm = currentRpm;
  //used to set the oil pressure status amungst other things
  checkIsRunning();
}
float getSpeedo(){
  return speed;
}
void putSpeedo(float currentSpeed){
  speed = currentSpeed;
}
unsigned long getOdometer(){
  return odometer;
}
void putOdometer(unsigned long currentOdometer){
  odometer = currentOdometer;
}
float getOilPressure(){
  return oilPressure;
}
void putOilPressure(float currentOilPressure){
  oilPressure = currentOilPressure;
  checkOilFault(oilPressure);
}
float getCoolantTemp(){
  return coolantTemp;
}
void putCoolantTemp(float currentCoolantTemp){
  coolantTemp = currentCoolantTemp;
  checkCoolantFault(currentCoolantTemp);
}
unsigned int getFuelLevel(){
  return fuelLevel;
}
void putFuelLevel(unsigned int level){
  fuelLevel = fuelLevel;
  checkFuelLevel(level);
}
float getBatteryVoltage(){
  return batteryVoltage;
}
void putBatteryVoltage(float volts){
  batteryVoltage = volts;
  checkBatteryVoltage(volts);
}


//boolean putters and getters

boolean getIsRunning(){
  return isRunning;
}
void putIsRunning(boolean run){
  isRunning = run;
}
boolean getIsBacklightOn(){
  return isBacklightOn;
}
void putIsBacklightOn(boolean backlight){
  isBacklightOn = backlight;
}
boolean getIsLowWasher(){
  return isLowWasher;
}
void putIsSecurity(boolean security){
  isSecurity = security;
}
boolean getIsSecurity(){
  return isSecurity;
}
void putIsCruiseActive(boolean cruiseStatus){
  isCruiseActive = cruiseStatus;
}
boolean getIsCruiseActive(){
  return isCruiseActive;
}
void putIsHatchAjar(boolean hatchAjar){
  isHatchAjar = hatchAjar;
}
boolean getIsHatchAjar(){
  return isHatchAjar;
}
void putIsEbrakeSet(boolean brakeStatus){
  isEBrakeSet = brakeStatus;
}
boolean getIsEbrakeSet(){
  return isEBrakeSet;
}
void putIsABSError(boolean aBSError){
  isABSError = aBSError;
}
boolean getIsABSError(){
  return isABSError;
}
void putIsTurnRight(boolean turnRight){
  isTurnRight = turnRight;
}
boolean getIsTurnRight(){
  return isTurnRight;
}
void putIsTurnLeft(boolean turnLeft){
  isTurnLeft = turnLeft;
}
boolean getIsTurnLeft(){
  return isTurnLeft;
}
void putIsHightBeam(boolean highBeamStatus){
  isHighBeam = highBeamStatus;
}
boolean putIsHighBeam(){
  return isHighBeam;
}
void putIsSeatBelt(boolean seatBeltStatus){
  isSeatBelt =  seatBeltStatus;
}
boolean getIsSeatBelt(){
  return isSeatBelt;
}
void putIsCELSet(boolean cELStatus){
  isCELSet = cELStatus;
}
boolean getIsCELSet(){
  return isCELSet;
}
void putIsCheckGauge(boolean checkGaugeStatus){
  isCheckGaugeSet = checkGaugeStatus;
  //sound the buzzer and set the light HIGH if true
  //unset light if false
}
boolean getIsCheckGauge(){
  return isCheckGaugeSet;
}
void putIsAirbagError(boolean airbagStatus){
  isAirbagError = airbagStatus;
}
boolean getIsAirbagSet(){
  return isAirbagError;
}
void putIsFuelLevelLow(boolean currentIsFuel){
  isFuelLevelLow = true;
}
boolean getIsFuelLevelLow(){
  return isFuelLevelLow;
}
void putIsFulltime4wd(boolean fullTime){
  isFulltime4wd = fullTime;
}
boolean getIsFulltime4wd(){
  return isFulltime4wd;
}
void putIsParttime4wd(boolean partTime){
  isParttime4wd = partTime;
}
boolean getIsParttime4wd(){
  return isParttime4wd;
}

//Test ranges

//counter of the number of errors
void incrementCheckGauge(){
  checkGaugeCount++;
  if (getIsCheckGauge() == false){
    putIsCheckGauge(true);
  }
}
void deincrementCheckGauge(){
  checkGaugeCount--;
  if (getIsCheckGauge()&&checkGaugeCount==0){
    putIsCheckGauge(false);
  }
}

//here are our tests (low oil pressure for example)
//oil pressure
void checkOilFault(float currentOilPressure){
  //check if the oil pressure is below range but only care if the engine is running
  if((currentOilPressure <= oilPressureLow)&&(getIsRunning)){
    //check if this is the first occurance
    if (oilPressureFault = false){
      //set the status and turn the light on
      oilPressureFault = true;
      incrementCheckGauge();
    }
    }else{
    //if the oil pressure status is flagged, unset it
    if (oilPressureFault){
      oilPressureFault = false;
      deincrementCheckGauge();
    }
  }
}

//coolant
void checkCoolantFault(float currentCoolant){
  //lets look at coolant temps, is it greater than the test
  if(currentCoolant >= coolantTempHigh){
    if (coolantTempFault = false){
      //this is the first occurance, lets set the flag and turn the light on
      coolantTempFault=true;
      incrementCheckGauge();
      }
    }else{
      if (oilPressureFault){
      //all is well now, unset the flag and deincrementCheckGauge();
        coolantTempFault=false;
        deincrementCheckGauge();
    }
  }
}

//fuel level
void checkFuelLevel(unsigned int currentFuelLevel){
  if (getFuelLevel() < fuelLevelLow){
    if (getIsFuelLevelLow() != true){
      putIsFuelLevelLow(true);
    }
    }else{
    if (getIsFuelLevelLow()){
      putIsFuelLevelLow(false);
    }
  }
}

//battery
void checkBatteryVoltage(float currentBattVolts){
  //if currentBattVolts is greater than batteryVoltageHigh or lower than batteryVoltageLow
  if((currentBattVolts>batteryVoltageHigh)||(currentBattVolts<batteryVoltageLow)){
    if (batteryVoltageFault=false){
      batteryVoltageFault = true;
      incrementCheckGauge();
    }
    }else{
    batteryVoltageFault = false;
    deincrementCheckGauge();
  }
}

//we only need to establish if the engine is running for stuff like oil pressure, maybe voltage if its an issue down the road
void checkIsRunning(){
  if ( getRPM() > 0){
    putIsRunning(true);
  }else{
    putIsRunning(false);
  }
}

//At some point i need to measure the voltage of the backlight to address the backlight LEDs

//MAIN PROGRAM

int tachWrite(int rpm_int) { // takes a integer RPM value and sends to Melexis
  wordTach = int(float((rpm_int+rpmOffset) % rpmPerQuad )/2.64);
  quadTach = rpm_int / rpmPerQuad ;
  wordTach = (wordTach << 2) | quadTach;
  testValueA = maskTach | highByte(wordTach);
  testValueB = lowByte(wordTach);
  digitalGaugeWrite(testValueA, testValueB, melexisSelectPin1);
}

// Function to write to the Melexis over SPI for any two bytes
int digitalGaugeWrite(byte testValueA, byte testValueB, int chipSelectPin) {
  digitalWrite(chipSelectPin,HIGH);
  // wait one clock cycle
  delayMicroseconds(2);
  // transfer the first half of the two byte command
  SPI.transfer(testValueA);
  // transfer the second half
  SPI.transfer(testValueB);
  // Set the SS pin low to deactivate the chip
  digitalWrite(chipSelectPin,LOW); 
}

void canRead(){
  if(CAN_MSGAVAIL == CAN.checkReceive()) // check if data coming
  {
    CAN.readMsgBuf(&len, buf); // read data, len: data length, buf: data buf

    canId = CAN.getCanId(); // store the CAN ID to a variable
    if(canId == 640) // if the CAN ID is from the ECM
      {
        rpm_int = 64 * ( buf[3]); // Convert byte 3 to revs/minute
        if(rpm_int<0){ rpm_int=0;} // don't allow negative rpms
      }
  }
  // Write rpm_int to the chip for Tach
  putRPM(rpm_int);
}

void initGauges(){
  Serial.begin(115200);

  while (can_init){
    if(CAN_OK == CAN.begin(CAN_500KBPS)) // init can bus : baudrate = 500k
    {
      Serial.println("CAN BUS Shield init ok!");
      can_init = true;
    }else{
      Serial.println("CAN BUS Shield init fail");
      Serial.println("Init CAN BUS Shield again");
      delay(100);
      //goto START_INIT;
    }
  }
  // Initialize SPI bus for Melexis
  SPI.begin(); 
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV32);
  // set the pin modes
  pinMode (melexisSelectPin1, OUTPUT);
  pinMode (selectPinMISO, INPUT);
  pinMode (selectPinMOSI, OUTPUT);
  pinMode (clockPin, OUTPUT);
  pinMode (ledPin, OUTPUT);
  pinMode (resetPin, OUTPUT);

  // Set "at-rest" states for SS, clock and LED pins
  digitalWrite(melexisSelectPin1,LOW);
  digitalWrite(clockPin, LOW);
  digitalWrite(ledPin,LOW);
  digitalWrite(resetPin,HIGH);
  //wait
  delay(1000);

  // Perform a sweep of the tach to home the needle
  // We need to work on this, and cycle all gauges at the same rate. 
  // Instead of sending rpm's (unit of measure specific) we could send the actual frations of the gauge's range
  for(int x = 8000; x >0; x--) {
    tachWrite(x);
  }
}

void pollStati(){
  //run ALL of the putters
  canRead();
  //putRPM(0) moved to canRead()
  putSpeedo(0.0);//can
  putOdometer(0.0);//can?
  putOilPressure(0.0);//can
  putCoolantTemp(0.0);//can
  putFuelLevel(0);//can?
  putBatteryVoltage(0.0);//can
  putBacklightIntensity(int(getBatteryVoltage()/0)); //replace 0 with the backlight voltage
  putIsSecurity(false);//dont think I will implement this
  putIsCruiseActive(false);//this will be set by a boolean that doesnt exist yet
  putIsHatchAjar(false);//analog
  putIsEbrakeSet(false);//analog
  putIsABSError(false);//analog
  putIsTurnRight(false);//analog
  putIsTurnLeft(false);//analog
  putIsHightBeam(false);//analog
  putIsSeatBelt(false);//analog
  putIsCELSet(false);//can
  putIsAirbagError(false);//ccd bus
  putIsFulltime4wd(false);//analog
  putIsParttime4wd(false);//analog
}
void updateGuage(){
  //run ALL of the gitters, then set the gauges and status lights
  /*
  if getIsBacklightOn(){
  //set the backlight to getBacklightIntensity()
  }*/
  tachWrite(getRPM());
  /*
  getSpeedo()
  getOdometer()
  getOilPressure()
  getCoolantTemp()
  getFuelLevel()
  getBatteryVoltage()
  getIsBacklightOn()
  getIsLowWasher()
  getIsSecurity()
  getIsCruiseActive()
  getIsHatchAjar()
  getIsEbrakeSet()
  getIsABSError()
  getIsTurnRight()
  getIsTurnLeft()
  getIsSeatBelt()
  getIsCELSet()
  getIsCheckGauge()
  getIsAirbagSet()
  getIsFuelLevelLow()
  getIsFulltime4wd()
  getIsParttime4wd()
  */
}

void setup() {
  // put your setup code here, to run once:
  initGauges();
}

void loop() {
  // put your main code here, to run repeatedly:
  pollStati();
  updateGuage();
}
