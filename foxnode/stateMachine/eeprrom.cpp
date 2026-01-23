//
// EEPROM code. Variables that are used to configure a given device
// and FUTURE subroutines to manage storage and retrieve from the RTC EEPROM
// virutally all of these variables are 1st initalized from #defines in various
// software sources and then after that in the setup() function they are retrieved
// from EEPROM. FUTURE hoepfully we can have a way to both set and retrive values
// in the 32 byte RTC EEPROM to manage these values.
// 23mar2025 PDH

// NOTE all time/delay units are mS

float thisFoxNodeLat;		// the lattitude of this fox node, either default or soon from EEPROM. Can be 0 if not known.
float thisFoxNodeLng;		// the longitude
float thisFoxNodeElev;		// the MSL Elevation
int rssiConnectValue;	// must be above this signal strenght to connect to server Wifi network
unsigned int blindTimeDelay;	// how many milliseconds we wait between sucessfull sensor value sumbissions to the server
unsigned int sensorSampleRate;	// current sample rate in 1 mS interval. (60000 = 1 minute)
								// Initilized from SENSOR_SAMPLE_RATE
unsigned short thisFoxNodeId;	// the ID number for this device

// Set up the eeprom variabels to their default values if there is a probelm with the RTC
// this replaced a number of #defines in various .h files
void eepromSetVariablesToDefault(void){
	thisFoxNodeId = 6;                  // This is where you manually set the FoxNode ID, Do not assign values greater than 175.
	thisFoxNodeLat = 64.83;  thisFoxNodeLng = -147.77;   thisFoxNodeElev = 137;
	rssiConnectValue = -55;		// signed, dBm. Drop threshold is 30 dB down from this.
	blindTimeDelay = 1500;		// typically 30 seconds, can be less for testing.
	sensorSampleRate = 3000;	// typicall 60,000 = 1 minute, can be less for testing.
}

// Read the RTC EEPROM and set the above varaibeles.
void eepromXferRtcToVariables(void){
	//
	// exctract EEPROM and set variables here...
	//
	if(thisFoxNodeId > 175)thisFoxNodeId = 0;		// We add 80 in httpComms.cpp, this safegaurds to make sure it's not over 175
}

void eepromXferVariablesToRtc(void){

}
