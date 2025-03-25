#ifndef EEPROML_H
#define EEPROM_H

// SCOPE: manage the EEPROM system that has specific per-FoxNode sensor client configuration

// This included the defintions for the variables and the subroutines to accces the EEPROM

// First the variables
extern unsigned short thisFoxNodeId;	// the ID number for this device
extern float thisFoxNodeLat;		// the lattitude of this fox node, either default or soon from EEPROM. Can be 0 if not known.
extern float thisFoxNodeLng;		// the longitude
extern float thisFoxNodeElev;		// the MSL Elevation
extern int rssiConnectValue;	// must be above this signal strenght to connect to server Wifi network
extern unsigned int blindTimeDelay;	// how many milliseconds we wait between sucessfull sensor value sumbissions to the server
extern unsigned int sensorSampleRate;	// current sample rate in 10 mS interval

extern void eepromSetVariablesToDefault(void);
extern void eepromXferRtcToVariables(void);
extern void eepromXferVariablesToRtc(void);

#endif	//	EEPROM_H
