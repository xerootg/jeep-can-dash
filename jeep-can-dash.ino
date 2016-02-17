#include <SPI.h>
#include "mcp_can.h"

/*
wishlist:
TPMS - https://github.com/jboone/tpms
Gas Mileage
*/

//non-boolean values
int rpm = 0;
unsigned long odometer = 0;
float speed = 0.0;
float oilPressure = 0.0;
float oilTemp = 0.0;
float coolantTemp = 0.0;
float ambientTemp = 0.0;
word fuelLevel = 0;
float batteryVoltage = 0.0;

//can addresses
const word motor1_id = 0x280;
const word motor2_id = 0x288;
const word kombi1_id = 0x320;
const word motor3_id = 0x380;
const word gra_id = 0x388;
const word kombi2_id = 0x420;
const word motor5_id = 0x480;
const word motor6_id = 0x488;
const word motor7_id = 0x588;

//can xmit bytes
word can_byte0 = 0x0;
word can_byte1 = 0x0;
word can_byte2 = 0x0;
word can_byte3 = 0x0;
word can_byte4 = 0x0;
word can_byte5 = 0x0;
word can_byte6 = 0x0;
word can_byte7 = 0x0;

//smoothing values (averaging) for temperature readings
//oil temp
const int oilNumReadings = 10;
int oilReadings[oilNumReadings];      // the readings from the analog input
int oilReadIndex = 0;              // the index of the current reading
int oilTotal = 0;                  // the running total
int oilInputPin = A0;

//coolant temp
const int waterNumReadings = 10;
int waterReadings[waterNumReadings];      // the readings from the analog input
int waterReadIndex = 0;              // the index of the current reading
int waterTotal = 0;                  // the running total
int waterInputPin = A1;

//ambient temp
const int ambientNumReadings = 10;
int ambientReadings[ambientNumReadings];      // the readings from the analog input
int ambientReadIndex = 0;              // the index of the current reading
int ambientTotal = 0;                  // the running total
int ambientInputPin = A2;

// here are our boolean types
boolean isRunning = false;
boolean isLowWasher = false;

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
float oilTempHigh = 220.0;
boolean oilTempFault = false;
float coolantTempHigh = 220.0;
boolean coolantTempFault = false;
word fuelLevelLow = 10;
//boolean fuelLevelFault = false; ---- this is a global above, with a getter and setter
float batteryVoltageLow = 12.0;
float batteryVoltageHigh = 16.0;
boolean batteryVoltageFault = false;

// CAN setup
const int SPI_CS_PIN = 9;
MCP_CAN CAN(SPI_CS_PIN);
unsigned char flagRecv = 0;
boolean init_can = false;

// here are our non-boolean value getters and setters
int getRPM(){
  return rpm;
}
void setRPM(int currentRpm){
  rpm = currentRpm;
  //used to set the oil pressure status amungst other things
  checkIsRunning();
}
float getSpeedo(){
  return speed;
}
void setSpeedo(float currentSpeed){
  speed = currentSpeed;
}
unsigned long getOdometer(){
  return odometer;
}
void setOdometer(unsigned long currentOdometer){
  odometer = currentOdometer;
}
float getOilPressure(){
  return oilPressure;
}
void setOilPressure(float currentOilPressure){
  oilPressure = currentOilPressure;
  checkOilFault(oilPressure);
}
float getOilTemp(){
  return oilTemp;
}
void setOilTemp(float currentOilTemp){
	//remove prior value from total
	oilTotal = oilTotal - oilReadings[oilReadIndex];
	//set the current value
	oilReadings[oilReadIndex] = currentOilTemp;
	//set the total
	oilTotal = oilTotal + oilReadings[oilReadIndex];
	
	//index math
	oilReadIndex = oilReadIndex + 1;
	if (oilReadIndex >= oilNumReadings) {
		oilReadIndex = 0;
	}
	
	// calculate the average and set it
	oilTemp = oilTotal / oilNumReadings;
	
	//make sure the average is within normal ranges
	checkOilTempFault(getOilTemp());
}
float getCoolantTemp(){
  return coolantTemp;
}
void setCoolantTemp(float currentCoolantTemp){
	waterTotal = waterTotal - waterReadings[waterReadIndex];
	waterReadings[waterReadIndex] = currentCoolantTemp;
	waterTotal = waterTotal + waterReadings[waterReadIndex];
	
	//index math
	waterReadIndex = waterReadIndex + 1;
	if (waterReadIndex >= waterNumReadings) {
		waterReadIndex = 0;
	}
	
	// calculate the average and set it
	coolantTemp = waterTotal / waterNumReadings;
	
	//make sure the average is within normal ranges
	checkCoolantFault(coolantTemp);
}
float getAmbientTemp(){
  return ambientTemp;
}
void setAmbientTemp(float currentAmbientTemp){
	ambientTotal = ambientTotal - ambientReadings[ambientReadIndex];
	ambientReadings[ambientReadIndex] = currentAmbientTemp;
	ambientTotal = ambientTotal + ambientReadings[ambientReadIndex];
	
	//index math
	ambientReadIndex = ambientReadIndex + 1;
	if (ambientReadIndex >= ambientNumReadings) {
		ambientReadIndex = 0;
	}
	
	// calculate the average and set it
	ambinetTemp = ambientTotal / ambientNumReadings;
}
word getFuelLevel(){
  return fuelLevel;
}
void setFuelLevel(word level){
  fuelLevel = fuelLevel;
  checkFuelLevel(level);
}
float getBatteryVoltage(){
  return batteryVoltage;
}
void setBatteryVoltage(float volts){
  batteryVoltage = volts;
  checkBatteryVoltage(volts);
}


//boolean setters and getters

boolean getIsRunning(){
  return isRunning;
}
void setIsRunning(boolean run){
  isRunning = run;
}
boolean getIsLowWasher(){
  return isLowWasher;
}
void setIsCruiseActive(boolean cruiseStatus){
  isCruiseActive = cruiseStatus;
}
boolean getIsCruiseActive(){
  return isCruiseActive;
}
void setIsHatchAjar(boolean hatchAjar){
  isHatchAjar = hatchAjar;
}
boolean getIsHatchAjar(){
  return isHatchAjar;
}
void setIsEbrakeSet(boolean brakeStatus){
  isEBrakeSet = brakeStatus;
}
boolean getIsEbrakeSet(){
  return isEBrakeSet;
}
void setIsABSError(boolean aBSError){
  isABSError = aBSError;
}
boolean getIsABSError(){
  return isABSError;
}
void setIsTurnRight(boolean turnRight){
  isTurnRight = turnRight;
}
boolean getIsTurnRight(){
  return isTurnRight;
}
void setIsTurnLeft(boolean turnLeft){
  isTurnLeft = turnLeft;
}
boolean getIsTurnLeft(){
  return isTurnLeft;
}
void setIsHightBeam(boolean highBeamStatus){
  isHighBeam = highBeamStatus;
}
boolean setIsHighBeam(){
  return isHighBeam;
}
void setIsSeatBelt(boolean seatBeltStatus){
  isSeatBelt =  seatBeltStatus;
}
boolean getIsSeatBelt(){
  return isSeatBelt;
}
void setIsCELSet(boolean cELStatus){
  isCELSet = cELStatus;
}
boolean getIsCELSet(){
  return isCELSet;
}
void setIsCheckGauge(boolean checkGaugeStatus){
  isCheckGaugeSet = checkGaugeStatus;
  //sound the buzzer and set the light HIGH if true
  //unset light if false
}
boolean getIsCheckGauge(){
  return isCheckGaugeSet;
}
void setIsAirbagError(boolean airbagStatus){
  isAirbagError = airbagStatus;
}
boolean getIsAirbagSet(){
  return isAirbagError;
}
void setIsFuelLevelLow(boolean currentIsFuel){
  isFuelLevelLow = true;
}
boolean getIsFuelLevelLow(){
  return isFuelLevelLow;
}
void setIsFulltime4wd(boolean fullTime){
  isFulltime4wd = fullTime;
}
boolean getIsFulltime4wd(){
  return isFulltime4wd;
}
void setIsParttime4wd(boolean partTime){
  isParttime4wd = partTime;
}
boolean getIsParttime4wd(){
  return isParttime4wd;
}

void incrementCheckGauge(){
  checkGaugeCount++;
  if (getIsCheckGauge() == false){
    setIsCheckGauge(true);
  }
}
void deincrementCheckGauge(){
  checkGaugeCount--;
  if (getIsCheckGauge()&&checkGaugeCount==0){
    setIsCheckGauge(false);
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
  if(getCoolantTemp >= coolantTempHigh){
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
void checkOilTempFault(float thisOilTemp){
  //lets look at coolant temps, is it greater than the test
  if(thisOilTemp) >= oilTempHigh){
    if (!oilTempFault){
      //this is the first occurance, lets set the flag and turn the light on
      oilTempFault=true;
      incrementCheckGauge();
      }
    }else{
      if (oilPressureFault){
      //all is well now, unset the flag and deincrementCheckGauge();
        oilTempFault=false;
        deincrementCheckGauge();
    }
  }
}
//fuel level
void checkFuelLevel(word currentFuelLevel){
  if (getFuelLevel() < fuelLevelLow){
    if (getIsFuelLevelLow() != true){
      setIsFuelLevelLow(true);
    }
    }else{
    if (getIsFuelLevelLow()){
      setIsFuelLevelLow(false);
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
    setIsRunning(true);
  }else{
    setIsRunning(false);
  }
}

//MAIN PROGRAM
void canRead(){
	if(flagRecv)                   // check if get data
	{

	flagRecv = 0;                // clear flag
	CAN.readMsgBuf(&len, buf); // read data, len: data length, buf: data buf

	canId = CAN.getCanId(); // store the CAN ID to a variable
	if(canId == motor1_id) //Motor 1
	  {
		//get rpm high, if 0xff then error
		//get rmp low, if 0xff then error
		setRPM(rpm_int);
      }
	if(canId == motor2_id) //Motor 2
      {
		buf[2]
		//get health bits, byte 0x2 
			//0x5 is freezeframe available
			//0x6-7 == 01 cruise is available
		buf[3]
		//get vehicle speed (low precision) byte 0x3
		buf[4]
		//get cruise target speed (low precision) byte 0x4
      }
	if(canId == motor3_id) //Motor 3
      {
		//get IAT 0x2
		//get motor3_bits, byte 0x0
			//preheat needed
      }
	if(canId == gra_id) //GRA
      {
		//nothing to get for now
		setIsCruiseActive(false);//this is a can recieved value.
      }
	if(canId == motor5_id) //Motor 5
      {
		//get info lamps, byte 0x1
			//glowplug 0x1
			//MIL 0x3
			setIsCELSet(false);
		//get fuel consumption high, byte 0x3
		//get fuel consumption low, byte 0x2
		//get info lamps B, 0x6
			//Cruise status, 0x2
			//mil type, 0x4-0x7
				//0x0000 - no text
				//0x0001 - engine
				//0x0010 - emissions
      }
	if(canId == motor6_id) //Motor 6
      {
		//nothing to get for now
      }
	if(canId == motor7_id) //Motor 7
      {
		//get alternator load, byte 0x1
      }
  }
  
}

word fToHex(tempInF){
	
	// NON FUNCTIONAL YET
	return tempInF;
}

void kombi1Msg(){
	/*
	0x0 status lamp status, first 6 bits are not used
		b0 n/a
		b1 n/a
		b2 n/a
		b3 n/a
		b4 n/a
		b5 n/a
		b6 [low fuel quantity]
		b7 [glowplug lamp enabled]
	*/
	can_byte0 = 0x0;
	/*
	0x1
	*/
	can_byte1 = 0x0;
	/*
	0x2
	*/
	can_byte2 = 0x0;
	/*
	0x3
	*/
	can_byte3 = 0x0;
	/*
	0x4
	*/
	can_byte4 = 0x0;
	/*
	0x5
	*/
	can_byte5 = 0x0;
	/*
	0x6
	*/
	can_byte6 = 0x0;
	/*
	0x7
	*/
	can_byte7 = 0x0;
	
	unsigned char stmp[8] = {can_byte0, can_byte1, can_byte2, can_byte3, can_byte4, can_byte5, can_byte6, can_byte7};
	CAN.sendMsgBuf(0x320, 0, 8, stmp);
}

void kombi2Msg(){
	/*
	0x0 temp sens error
		b0 ambient temp sens error
		b1 oil temp error
		b2 water temp error
		b3-7 = 0
	*/
	can_byte0 = 0x0;
	/*
	1 outside temp, weighted
		float getAmbientTemp()
	*/
	can_byte1 = fToHex(getAmbientTemp());
	/*
	2 outside temp
	*/
	can_byte2 = fToHex(getAmbientTemp());
	/*
	3 oil temp
		float getOilTemp()
	*/
	can_byte3 = fToHex(getOilTemp);
	/*
	4 coolant temp
		float getCoolantTemp()
	5 backlight error = 0
	6 backlight error = 0
	*/
	can_byte4 = 0x0;
	can_byte5 = 0x0;
	can_byte6 = 0x0;
	
	unsigned char stmp[8] = {can_byte0, can_byte1, can_byte2, can_byte3, can_byte4, can_byte5, can_byte6};
	CAN.sendMsgBuf(0x420, 0, 7, stmp);
}

void canWrite{
	kombi1Msg();
	kombi2Msg();
}


// READ ANALOG THINGS and SET THEM
void readOilTemp(){
	//code to read the analog value in degrees F
	float recievedValue = 0.0;
	setOilTemp(recievedValue);
}
void initOilTemp(){
	for (int thisReading = 0; thisReading < oilNumReadings; thisReading++) {
		oilReadings[thisReading] = 0;
	}
}
void readAmbientTemp(){
	//code to read the analog value in degrees F
	float recievedValue = 0.0;
	setAmbientTemp(recievedValue);
}
void initAmbientTemp(){
	for (int thisReading = 0; thisReading < ambientNumReadings; thisReading++) {
		ambientReadings[thisReading] = 0;
	}
}
void readCoolantTemp(){
	//code to read the analog value in degrees F
	float recievedValue = 0.0;
	setCoolantTemp(recievedValue);
}
void initCoolantTemp(){
	for (int thisReading = 0; thisReading < waterNumReadings; thisReading++) {
		waterReadings[thisReading] = 0;
	}
}
void readSpeedInfo(){
	//counter, time and all that stuff equal speed and distance
	setSpeedo(0.0);
	setOdometer(0.0);
}
void readOilPressure(){
	//read the analog oil pressure and set it in PSI
	setOilPressure(0.0);
}
void readFuelLevel(){
	//set the fuel level in leters from analog value
	setFuelLevel(0);//can?
	//fuel level warning is set by checkFuelLevel() in setFuelLevel()
}
void readBatteryVolatge(){
	setBatteryVoltage(0.0);
}


void initGauges(){
	Serial.begin(115200);
	
	//docs use a goto. I hate goto's. Lets use a while not do
	
	while (! init_can) {                  // init can bus : baudrate = 500k
		if (CAN_OK == CAN.begin(CAN_500KBPS)){
			init_can = true;
		} else {
			Serial.println("CAN BUS Shield init fail");
			Serial.println("Init CAN BUS Shield again");
			delay(100);
		}
	}
	
	Serial.println("CAN BUS Shield init ok!");
	attachInterrupt(0, MCP2515_ISR, FALLING); // start interrupt
 }
 
void MCP2515_ISR(){
    flagRecv = 1;
}

void pollStati(){
  //run ALL of the setters
  canRead();
  //setRPM(0) moved to canRead()
  
  readSpeedInfo()
  
   //analog value
  readOilPressure()
  
  readOilTemp(); //calls setOilTemp() and adds buffering
  readAmbientTemp(); //calls setAmbientTemp() and adds buffering
  readWaterTemp(); //calls setWaterTemp() and adds buffering
  readFuelLevel();
  readBatteryVolatge();
  
  setIsHatchAjar(false);//analog
  setIsEbrakeSet(false);//analog
  setIsABSError(false);//analog
  setIsTurnRight(false);//analog
  setIsTurnLeft(false);//analog
  setIsHightBeam(false);//analog
  setIsSeatBelt(false);//analog
  setIsAirbagError(false);//ccd bus
  setIsFulltime4wd(false);//analog
  setIsParttime4wd(false);//analog
}
void updateGuage(){
  //run ALL of the gitters, then set the gauges and status lights

  /*
  getRPM()
  getSpeedo()
  getOdometer()
  getOilPressure()
  getCoolantTemp()
  getFuelLevel()
  getBatteryVoltage()
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
  initOilTemp();
  initAmbientTemp();
  initCoolantTemp();
  initGauges();
}

void loop() {
  // put your main code here, to run repeatedly:
  pollStati();
  updateGuage();
  canWrite(); //writes values just parsed into CAN DATA for the ECU to consume
}
