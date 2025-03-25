#ifndef SENSOR_H
#define SENSOR_H

// SCOPE: Globals for the external sensor PCB. 4 sensors (lux, temp, humidty, 3 axis accelerometer) and RTC chip with battery back-up

// Includes
#include <Arduino.h>
#include <SPI.h>
#include <stdio.h>                              // This will allow the usage of sprintf()
#include <ArduinoJson.h>                        // Include the ArduinoJson library
#include "serial.h"
#include "timer.h"
#include "state.h"
#include "eeprom.h"
#include <Wire.h>							                  // Used in all "ClosedCube" sensor libs
#include <ClosedCube_OPT3001.h>			  	        // Lux I2C sensor
#include "ClosedCube_LPS25HB.h"				          // Preassure [mBar] and Temp [C]
#include "kxtj3-1057.h" 					              // Include the KXTJ3-1057 library (includes Wire.h)
#include <NTPClient.h>									        // NTP web client for FN init              
#include <time.h>										            // For struct tm and localtime()
#include <bb_rtc.h>										          // Driver RTC RV-3032 NOTE: defines are here but rtc object is lives in timer .h/.cpp

// Global Variables
#define OPT3001_ADDRESS 0x44                    // Lux I2C addr def
#define LPS25HB_ADDRESS 0x5C                    // Preassure I2C addr def
#define KXTJ3_ADDRESS 0x0E                      // Acc. I2C addr def
//// Acc. sensor configuration
#define KXTJ3_SAMPLERATE 1000                   // Sample rate in Hz (0.781, 1.563, 3.125, 6.25, ... , 1600Hz)
#define KXTJ3_ACCELRANGE 2                      // 14-bit Mode only supports 8g or 16g
#define KXTJ3_HIGHRES false                     // Set high resolution mode 
#define KXTJ3_THRESHOLD 100                     // Default wake-up threshold
#define KXTJ3_MOVEDUR 100                       // Default movement duration
#define KXTJ3_NADUR 0                           // Default non-activity duration
#define KXTJ3_POLARITY HIGH                     // INT pin polarity: HIGH or LOW
#define KXTJ3_WURATE -1                         // Wake-up sample rate (IMU sample rate)
#define KXTJ3_LATCHED false                     // Latched interrupt mode (true/false)
#define KXTJ3_PULSED false                      // Pulsed interrupt mode (true/false)
#define KXTJ3_MOTION true                       // Motion Detection interrupt enabled/disabled
#define KXTJ3_DATAREADY false                   // New Data Ready interrupt enabled/disabled
#define KXTJ3_INTPIN false                      // INT pin operation enabled/disabled
#define KXTJ3_DEBUGSET false                    // Serial debug messages enabled/disabled

extern JsonDocument core_doc;                   // JSON Doc to hold Fox-Node general HTTP-POST payloads (ccmd="hi") 
extern JsonDocument data_doc;                   // JSON Doc to hold sensor data, sent to Ringbuffer
extern JsonDocument postRxPayload_doc;          // JSON Doc to hold any recived POST payload reply (UAS --> FoX-Node)
extern DynamicJsonDocument httpDataPayloadDoc;  // Dynamic JSON Doc to pull RingBuffer data 
extern ClosedCube_OPT3001 opt3001;			        // Lux I2C sensor object
extern ClosedCube_LPS25HB lps25hb;			        // Preassure I2C sensor object
extern KXTJ3 myIMU; 
extern WiFiUDP ntpUDP;									        // ntpUDP client used for initial NTP read 
extern NTPClient timeClient;		                // Target NTP server
extern String formattedTime;						        // NTP var 
extern unsigned long previosMillis;			        // Time out var for dummy server Initial NTP WiFi look around
extern unsigned long timeout;					          // Time out var for dummy server Initial NTP WiFi look around 
extern struct tm myTime;								        // must implement a "tm struct" to get/set RTC time. However Unix time is for all transmisions [seconds]
extern BBRTC rtc;
extern const char* szRTCType[];
extern const char* szDays[];
extern unsigned int httpConnectState;
extern String httpParamPayload;								  // String to hold incoming POST reply data, deserialized into  postRxPayload_doc JSON doc
extern String httpDataPayload;								  // String to hold serialized data_doc JSON 
extern String coreDataJSON;                     // String to hold serialized core_doc JSON

String CoreDataAndMerge(String var, time_t myUnixTime1, time_t myUnixTime2);
String createCoreData(String var);              // fills out the 'core' data object and adds it to the POST payload
String getOneSetSensorValues(void);					    // Extracts the "data" object and adds it to the POST payload
String InitDummyData(void);

// Function Declarations
void initializeSensors(void);
void OPT3001_configureSensor(void);
void KXTJ3_configureSensor(void);
void LP522DF_configureSensor(void);
void HDC1080_configureSensor(void);
void LP522DF_read(void);
void KXTJ3_read(void);
void HDC1080_read(void);
void takeSaveSensorData(void);

#endif  // SENSOR_H
