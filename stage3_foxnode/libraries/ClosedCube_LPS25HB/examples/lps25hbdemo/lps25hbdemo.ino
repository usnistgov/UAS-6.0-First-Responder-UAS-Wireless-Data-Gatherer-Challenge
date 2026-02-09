/**************************************************************************************

This is example for ClosedCube LPS25HB Absolute Digital Barometer

Initial Date: 22-Jun-2016

Hardware connections for Arduino Uno:
VDD to 3.3V DC
SCL to A5
SDA to A4
GND to common ground

Written by AA for ClosedCube

MIT License

**************************************************************************************/

#include <Wire.h>
#include "ClosedCube_LPS25HB.h"

ClosedCube_LPS25HB lps25hb;

void setup()
{
	Serial.begin(9600);
	Serial.println("ClosedCube LPS25HB Arduino Test");
	
	lps25hb.begin(0x5C);

	Serial.print("Who Am I? 0x");
	Serial.print(lps25hb.whoAmI(), HEX);
	Serial.println(" (expected: 0xBD)");
}

void loop()
{
	Serial.print("P=");
	Serial.print(lps25hb.readPressure());
	Serial.print(" mbar, T=");
	Serial.print(lps25hb.readTemperature());
	Serial.println("C");
	delay(1000);
}

