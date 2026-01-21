#include "serial.h"

// Serial Port code
// There is the ability to output strings on the serial port
// This section also contains some "canned" reports that are
// part of the system.
// There is also a loging mechanizm that has a concept of priorty
//   or "verbose"-ness. There is also a "hook" for diferentiating
// between a write to the serial port vs. a put to a log file.
// It is envisioned that the log file could be a big ring buffer
// that discards data if written to when full

unsigned int serialLogLevel;				// what log level are we at.

void initializeSerial(void){
	serialLogLevel = 4;					// default out of boot is no logging
	Serial.begin(115200);				// Start Serial interface
	delay(1000);						// for serial setup
}

// Logging levels are:
//  0 - none      1 - errors   2 - general/high level info   3 - medium info   4 - 4 Very Verbos

void putSetSerialLogLevel(unsigned int level){
	if(level > 4){
		Serial.print("** putSetSerialLogLevel(): Passed level was too high, set it to 4 ");
		level = 4;
	}
	serialLogLevel = level;
}

// Note: we do not have a log buffer yet.
void putToDebug(String s, unsigned int logPriority){			// Level-savy put to the debug means, which is a serial port now.
	if(logPriority == 0) return;								// Skip if logPriority is 0. this may be a cirular ring buffer later
	if(logPriority <= serialLogLevel) Serial.print(s);			// Print the string using Serial.print
	return;
}

void putToDebugWithNewline(String s, unsigned int logPriority){	// Level-savy put to the debug means, which is a serial port now.
	if(logPriority == 0) return;								// Skip if logPriority is 0
	if(logPriority <= serialLogLevel) Serial.println(s);		// Print the string with a newline using Serial.println
	return;
}

void putToSerial(String s){					// Put a string to the serial port.
	Serial.print(s);						// Print the string using Serial.print
}

void putToSerialWithNewline(String s){		// Put a string to the serial port and terminate with a newline
	Serial.println(s);						// Print the string with a newline using Serial.println
}

