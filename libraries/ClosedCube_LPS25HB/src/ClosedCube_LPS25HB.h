/*

Arduino Library for STMicroelectronics LPS25HB MEMS pressure sensor: 260-1260 hPa absolute digital output barometer
Written by AA for ClosedCube
---

The MIT License (MIT)

Copyright (c) 2016 ClosedCube Limited

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/


#ifndef _CLOSEDCUBE_LPS25HB_h

#define _CLOSEDCUBE_LPS25HB_h
#include <Arduino.h>

class ClosedCube_LPS25HB {
public:
	ClosedCube_LPS25HB();

	void begin(uint8_t address);

	uint8_t whoAmI();
	float readTemperature();
	float readT(); // short-cut for readTemperature

	float readPressure();
	float readP(); // short-cut for readPressure

private:
	uint8_t _address;
	uint8_t read(uint8_t reg);
	void write(uint8_t reg, uint8_t data);
	uint8_t status(uint8_t data);
};

#endif


