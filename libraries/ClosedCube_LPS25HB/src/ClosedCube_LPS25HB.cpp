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

#include <Wire.h>
#include "ClosedCube_LPS25HB.h"

ClosedCube_LPS25HB::ClosedCube_LPS25HB()
{
}

void ClosedCube_LPS25HB::begin(uint8_t address) {
	_address = address;
	Wire.begin();
	write(0x10, 0xA); // resolution: temp=32, pressure=128
	write(0x20, 0x8C); // one-shot mode
}

byte ClosedCube_LPS25HB::whoAmI() {
	Wire.beginTransmission(_address);
	Wire.write(0x0F);
	Wire.endTransmission();
	Wire.requestFrom(_address, (uint8_t)1);
	return Wire.read();
}

float ClosedCube_LPS25HB::readP() {
	return readPressure();
}

float ClosedCube_LPS25HB::readPressure() {
	write(0x21, 0x1);

	if (status(0x2) < 0)
		return 0;

	uint8_t pressOutH = read(0x2A);
	uint8_t pressOutL = read(0x29);
	uint8_t pressOutXL = read(0x28);

	long val = ( ((long)pressOutH << 24) | ((long)pressOutL << 16) | ((long)pressOutXL << 8)) >> 8;
	return val/4096.0f;
}

float ClosedCube_LPS25HB::readT() {
	return readTemperature();
}

float ClosedCube_LPS25HB::readTemperature() {
	write(0x21, 0x1);
	if (status(0x1) < 0)
		return 0;

	uint8_t tempOutH = read(0x2C);
	uint8_t tempOutL = read(0x2B);

	int16_t val = tempOutH << 8 | tempOutL & 0xff;
	return 42.5f+val/480.0f;
}


uint8_t ClosedCube_LPS25HB::status(uint8_t status) {
	int count = 1000;
	uint8_t data = 0xff;
	do {
		data = read(0x27);
		--count;
		if (count < 0)
			break;
	} while ((data & status) == 0);

	if (count < 0)
		return -1;
	else
		return 0;
}

uint8_t ClosedCube_LPS25HB::read(uint8_t reg) {
	Wire.beginTransmission(_address);
	Wire.write(reg);
	Wire.endTransmission();
	Wire.requestFrom(_address, (uint8_t)1);
	return Wire.read();
}

void ClosedCube_LPS25HB::write(uint8_t reg, uint8_t data) {
	Wire.beginTransmission(_address);
	Wire.write(reg);
	Wire.write(data);
	Wire.endTransmission();
}





