#include "serial.h"
#include <WiFi.h>
#include <cstring>

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

// Set serial monitor for debugging.
//Reset Reasons
//| Value | Meaning          |
//| ----: | ---------------- |
//|     1 | Power-on reset   |
//|     3 | Software reset   |
//|     5 | Deep sleep reset |
//|     7 | Watchdog reset   |
//|     9 | Brownout         |
//|    12 | RTC watchdog     |

// Logging levels are:
//  0 - none      1 - errors   2 - general/high level info   3 - medium info   4 - 4 Very Verbose

void initializeSerial(void){
	serialLogLevel = 1;					// default out of boot is no logging, recommended level 1, 4 may risk overloading MCU
	Serial.begin(230400);			// Start Serial interface
	Serial.setRxBufferSize(2048); // Set buffer bigger for verbose logging
	delay(1000);						// for serial setup
	esp_reset_reason_t r = esp_reset_reason();
	Serial.printf("Reset reason: %d\n", r);
}

void putSetSerialLogLevel(unsigned int level){
	if(level > 4){
		Serial.print("** putSetSerialLogLevel(): Passed level was too high, set it to 4 ");
		level = 4;
	}
	serialLogLevel = level;
}

// Note: we do not have a log buffer yet.
void putToDebug(String s, unsigned int logPriority){			// Level-savy put to the debug means, which is a serial port now.
	//if(logPriority == 0) return;								// Skip if logPriority is 0. this may be a cirular ring buffer later
	if(logPriority <= serialLogLevel) Serial.print(s);			// Print the string using Serial.print
	return;
}

void putToDebugWithNewline(String s, unsigned int logPriority){	// Level-savy put to the debug means, which is a serial port now.
	//if(logPriority == 0) return;								// Skip if logPriority is 0
	if(logPriority <= serialLogLevel) Serial.println(s);		// Print the string with a newline using Serial.println
	return;
}

void putToDebugWithNewlineTrunc(const String &s, unsigned int logPriority, size_t maxChars){
  if(logPriority > serialLogLevel) return;

  size_t n = s.length();
  size_t take = (n > maxChars) ? maxChars : n;

  // Print in small chunks and yield so we don’t starve WiFi
  const size_t CHUNK = 64;
  for (size_t i = 0; i < take; i += CHUNK) {
    size_t c = (take - i > CHUNK) ? CHUNK : (take - i);
    Serial.write((const uint8_t*)s.c_str() + i, c);
    yield();
  }

  if (n > maxChars) {
    Serial.print("... (trunc ");
    Serial.print(n);
    Serial.println(" chars)");
  } else {
    Serial.println();
  }

  yield();
}


void putToSerial(String s){					// Put a string to the serial port.
	Serial.print(s);						// Print the string using Serial.print
}

void putToSerialWithNewline(String s){		// Put a string to the serial port and terminate with a newline
	Serial.println(s);						// Print the string with a newline using Serial.println
}

