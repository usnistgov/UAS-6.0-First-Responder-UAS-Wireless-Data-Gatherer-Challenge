#ifndef SERIAL_H
#define SERIAL_H

// SCOPE: Setup general Serial output and provide debug logging at varios verbosity.

// The serial port is used for logging and diagnostics
// Logging levels are:
//  0 - none      1 - errors   2 - general/high level info   3 - medium info   4 - 4 Very Verbos

// Includes 
#include <WiFi.h>                                               // For ESP32 or other Arduino-compatible Wi-Fi libraries
#include <WebServer.h>
#include <HTTPClient.h>
#include "httpComms.h" 
#include <Arduino.h>

// Global Variables
extern unsigned int serialLogLevel;		                          // what log level are we at
extern unsigned int wantToPaintDisplay;

// Function Declarations
void initializeSerial(void);
void putSetSerialLogLevel(unsigned int level);		              // subber to set current log level,
void putToDebug(String s, unsigned int logLevel);
void putToDebugWithNewline(String s, unsigned int logPriority);
void putToSerial(String s);
void putToSerialWithNewline(String s);

// Safe debug print: truncates and prints in small chunks with yield() to avoid WiFi starvation.
// Use this for large JSON strings.
void putToDebugWithNewlineTrunc(const String &s, unsigned int logPriority, size_t maxChars = 240);

#endif	//	SERIAL_H
