//
// Code for everything sensor based on the external sensor
// PCB. Measure Lux, Tempurature, Humidty, Pressure
// and a 3 axis accelerometer.

///// this seems like a good place for the RTC code too. This makes this moudle
///// be "everything on the I2C external sensor board" ????

#include "sensor.h"
#include "serial.h"
#include <NTPClient.h>                                          // NTP web client for FN init              
#include "Ringbuffer.h"                                         // for httpDataPayloadDoc
#include "eeprom.h"

#include <Wire.h>                                               // For I2C communication
#include <HDC1080.h>                                            // Include HDC1080 sensor library // DEBUG HDC lib... this is giving me trouble.... 

JsonDocument core_doc;		                                      // JSON Doc to hold Fox-Node general HTTP-POST payloads (ccmd="hi")  
JsonDocument data_doc;                                          // JSON Doc to hold sensor data, sent to Ringbuffer                  
JsonDocument postRxPayload_doc;                                 // JSON Doc to hold any recived POST payload reply (UAS --> FoX-Node)
DynamicJsonDocument httpDataPayloadDoc(1024);                   // Dynamic JSON Doc to hold pulled RingBuffer data //TODO size this to be max ring-buffer size.... ? not RING_BUFFER_SIZE! but the estimation output

// JSON payload String variables
String httpParamPayload = "";		                                // String to hold incoming POST reply data, deserialized into  postRxPayload_doc JSON doc
String httpDataPayload =  "";		                                // String to hold serialized data_doc JSON 
String coreDataJSON =  "";                                      // String to hold serialized core_doc JSON 

GuL::HDC1080 hdc1080(Wire);                                     // pressure & humidity sensor This lib is very frustrating... contains its own namespace... 

ClosedCube_OPT3001 opt3001;			                                // Lux I2C sensor object
ClosedCube_LPS25HB lps25hb;			                                // Preassure I2C sensor object
KXTJ3 myIMU(KXTJ3_ADDRESS);                                     // Accelerometer sesnor 

WiFiUDP ntpUDP;									                                // ntpUDP client used for initial NTP read
NTPClient timeClient(ntpUDP,"pool.ntp.org");	                  // Target NTP server


String formattedTime;				                                    // NTP time holder var
unsigned int unixTime;				                                  // derived from the formatedTime;
unsigned long previosMillis = 0;	                              // Time out var for dummy server Initial NTP WiFi look around
unsigned long timeout = 10000;		                              // Time out var for dummy server Initial NTP WiFi look around
struct tm myTime;					                                      // must implement a "tm struct" to get/set RTC time. However Unix time is for all transmisions [seconds]
BBRTC rtc;
const char* szRTCType[] = { "None", "PCF8563", "DS3231", "RV-3032" };
const char* szDays[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

//////////////// code to create the POST data, driven by the MSM  ////////////////////
// For the "srep" request from the drone, both the "core" and "data" objects are
// created as JSON strings. Those two JSON encoded data objects are then
// concatinated into the final HTTP POST transmit buffer to be sent.

String CoreDataAndMerge(String var, time_t myUnixTime1, time_t myUnixTime2) {
  core_doc.clear();                                                        // Good house keeping
  // Create the base "core" information
  JsonObject core = core_doc.createNestedObject("core");	// Create the base "core" information
  core["ccmd"] = var;										// Client command
  core["fox"] = thisFoxNodeId;								// FoxNode ID
  core["fip"] = WiFi.localIP();								// FoxNode fixed IP
  core["fts"] = mktime(&myTime);							// Current Unix time
  core["fbv"] = "F";										// Placeholder battery voltage
  core["fcnt"] = ringbuff.getCount();						// Number of sensor samples in memory
  core["ftso"] = ringbuff.getOldestTimestamp();				// Oldest timestamp
  core["ftsr"] = ringbuff.getNewestTimestamp();				// Most recent timestamp
  core["wrxs"] = WiFi.RSSI();								// WiFi signal strength
  core["lat"] = thisFoxNodeLat;										// All GPS vals are hard coded in "core.h/cpp". No GPS available currenlty
  core["lon"] = thisFoxNodeLng;										// All GPS vals are hard coded in "core.h/cpp". No GPS available currenlty
  core["elev"] = thisFoxNodeElev;										// All GPS vals are hard coded in "core.h/cpp". No GPS available currenlty
  
  DynamicJsonDocument ringBufferData = ringbuff.pullEntriesByTime(myUnixTime1, myUnixTime2);    // Fetch data from the RingBuffer

  JsonObject data = core_doc.createNestedObject("data");                   // Get the "data" section from the document  // Create the "data" object 

  JsonObject ringBufferDataObj = ringBufferData["data"].as<JsonObject>();  // Get data from the ring buffer
  
  data["fox"] = thisFoxNodeId;
  data["dlen"] = ringBufferDataObj["dlen"];                                // Number of entries pulled from Ringbuffer for data dump
  data["ftso"] = ringBufferDataObj["ftso"];                                // Oldest timestamp
  Serial.println("Sending "+String(data["dlen"])+" Data Entries\n");       // report how many Data entries found in Ringbuffer
  JsonArray dumpData = data.createNestedArray("dump");                     // Create the "dump" array to store sensor readings

  for (JsonObject entry : ringBufferDataObj["dump"].as<JsonArray>()) {     // Copy over the "dump" data from the ring buffer
    JsonObject dumpEntry = dumpData.createNestedObject();
    dumpEntry["rts"] = entry["rts"];
    dumpEntry["t"] = entry["t"];
    dumpEntry["c"] = entry["c"];
    dumpEntry["h"] = entry["h"];
    dumpEntry["l"] = entry["l"];
    dumpEntry["p"] = entry["p"];
    dumpEntry["x"] = entry["x"];
    dumpEntry["y"] = entry["y"];
    dumpEntry["z"] = entry["z"];
  }

  serializeJson(core_doc, coreDataJSON);                                   // Serialize the combined document for HTTP POST payload
  return coreDataJSON;
}

String getOneSetSensorValues(void) {
	data_doc.clear();                                                        // Reset our doc(make sure we havent malformed because of a data payload Tx)
  rtc.getTime(&myTime);                                                    // Update ESP32 clock with RTC val. "SNS-PCB" I2C connected Real Time Clock value +- 2ppm
	myUnixTime = mktime(&myTime);                                            // Update Unix time var 
	
	data_doc["rts"] = myUnixTime;                                            // relative time stamp
	data_doc["tpwr"] = WiFi.getTxPower();                                    // Write Fox-Node Tx power setting to Payload
	data_doc["rssi"] = WiFi.RSSI();      
	data_doc["ip"] = WiFi.localIP();                                         // Set in WiFi connection routine 
	HDC1080_read();                                                          // populate tests data_doc["t"] / data_doc["h"]  
	//// "t" HDC1080 Temperature in degrees [C]
	//// "h" HDC1080 Humidity, percent, integer
	data_doc["c"] = temperatureRead();                                       // ESP32-S2 Feather TFT CPU temp  NOTE: temperatureRead() is provided via Arduino.h lib 
	data_doc["l"] = opt3001.readResult().lux;                                // OPT3001 Lux light level, lumens, integer
	data_doc["p"] = lps25hb.readPressure();                                  // pressure, hPa, integer //TODO currently [mbar]
	KXTJ3_read();                                                            // Populate data_doc["x"]/data_doc["y"]/data_doc["z"]. Accelerometer KXTJ3 read. This chip wants you to wake it on read then put back to sleep. 
	//// "x" data_doc["x"] KXTJ3 accelerometer X mg, integer
	//// "y" data_doc["y"] KXTJ3 accelerometer Y mg, integer
	//// "z" data_doc["z"] KXTJ3 accelerometer Z mg, integer
	
	serializeJson(data_doc, httpDataPayload);                                // Write JSON HTTP-POST payload (global var)
  return httpDataPayload;
}

String createCoreData(String var) {
  core_doc.clear();                                                         // good hosue keeping 
  rtc.getTime(&myTime);                                                     // debug update ESP32 clock with RTC val // Read SNS-PCB Real Time Clock value +- 2ppm
  myUnixTime = mktime(&myTime);                                             // Update Unix time var

  JsonObject core = core_doc["core"].to<JsonObject>();
  core["ccmd"] = var;                                                       // client command, hardcoded for general POST payload
  core["fox"] = thisFoxNodeId;                                                      // FoxNode ID number
  core["fip"] = WiFi.localIP();                                             // FoxNode Sencor Client fixed IP
  core["fts"] = myUnixTime;                                                 // FoxNode
  core["fbv"] = "F";                                                        // FoxNode Battery Voltage //TODO... I have nothing to read :( this connector is un-used
  core["fcnt"] = ringbuff.getCount();                                       // Number of sensor samples in memory
  core["ftso"] = ringbuff.getOldestTimestamp();                             // 'o' Oldest timestamp of sensor values (in seconds)
  core["ftsr"] = ringbuff.getNewestTimestamp();                             // 'r' Most recent timestamp of sensor values (in seconds)
  core["wrxs"] = WiFi.RSSI();                                               // WiFi receive strength in [dBm]
  core["lat"] = thisFoxNodeLat;                                             // All GPS vals are hard coded in "core.h/cpp". No GPS available currenlty
  core["lon"] = thisFoxNodeLng;                                             // All GPS vals are hard coded in "core.h/cpp". No GPS available currenlty
  core["elev"] = thisFoxNodeElev;                                           // All GPS vals are hard coded in "core.h/cpp". No GPS available currenlty

  serializeJson(core_doc, coreDataJSON);                                    // Write JSON HTTP-POST payload (global var)
  return coreDataJSON;                                             
}

String InitDummyData(void) {  
	data_doc["rts"] = "F";                                                     // relative time stamp
	data_doc["tpwr"] = "F";                                                    // Fox-Node connected AP TX Power (if connected WiFi)
	data_doc["rssi"] = "F";                                                    // Fox-Node RSSI (if connected WiFi)    
	data_doc["ip"] = "F";                                                      // Fox-Node IP (if connected WiFi) 
	data_doc["t"] = "F";                                                       // LPS25HB Temperature in degrees F, integer //TODO [C]
	data_doc["c"]  = "F";                                                      // ESP32-S2 Feather TFT CPU temp 
	data_doc["h"]  = "F";                                                      // HDC1080 Humidity 
	data_doc["l"] = "F";                                                       // OPT3001 Lux light level, lumens, integer
	data_doc["p"] = "F";                                                       // pressure, hPa, integer //TODO currently [mbar]
	data_doc["x"] = "F";                                                       // KXTJ3 accelerometer X mg, integer
	data_doc["y"] = "F";                                                       // KXTJ3 accelerometer Y mg, integer
	data_doc["z"] = "F";                                                       // KXTJ3 accelerometer Z mg, integer
	
	serializeJson(data_doc, httpDataPayload);                                  // Write JSON HTTP-POST payload (global var)
  return httpDataPayload;
}

void initializeSensors(void){
	OPT3001_configureSensor();		                                             // Light/Lux
	KXTJ3_configureSensor();		                                               // 3 axis acclerometer for device orientation
	LP522DF_configureSensor();		                                             // pressure
	HDC1080_configureSensor();		                                             // tempurature/humidity
}

void OPT3001_configureSensor(void) {
  opt3001.begin(OPT3001_ADDRESS);
  putToDebug("OPT3001 Manufacturer ID", 3);
  char manufacturerIDStr[6];                                                     // Buffer to hold the string (enough for a 5-digit number + null terminator)
  sprintf(manufacturerIDStr, "%u", opt3001.readManufacturerID());                // Convert the uint16_t to a string
  putToDebug(manufacturerIDStr, 3);                                              // Pass the string to putToDebug
  putToDebug("OPT3001 Device ID", 3);
  char readDeviceID[6];                                                          // Buffer to hold the string (enough for a 5-digit number + null terminator)
  sprintf(readDeviceID, "%u", opt3001.readDeviceID());                           // Convert the uint16_t to a string
  putToDebug(readDeviceID, 3);                                                   // Pass the string to putToDebug
  OPT3001_Config newConfig;
  newConfig.RangeNumber = B1100;
  newConfig.ConvertionTime = B0;
  newConfig.Latch = B1;
  newConfig.ModeOfConversionOperation = B11;
  OPT3001_ErrorCode errorConfig = opt3001.writeConfig(newConfig);
  putToDebug("OPT3001 High-Limit: ", 3);
  char readHighLimit[6];                                                         // Buffer to hold the string (enough for a 5-digit number + null terminator)
  sprintf(readHighLimit, "%u", opt3001.readHighLimit());                         // Convert the uint16_t to a string
  putToDebug(readHighLimit, 3);                                                  // Pass the string to putToDebug
  putToDebug("OPT3001 Low-Limit: ", 3);
  char readLowLimit[6];                                                          // Buffer to hold the string (enough for a 5-digit number + null terminator)
  sprintf(readLowLimit, "%u", opt3001.readLowLimit());                           // Convert the uint16_t to a string
  putToDebug(readLowLimit, 3);                                                   // Pass the string to putToDebug
}

void KXTJ3_configureSensor(void) {
  if (myIMU.begin(KXTJ3_SAMPLERATE, KXTJ3_ACCELRANGE, KXTJ3_HIGHRES, KXTJ3_DEBUGSET) == IMU_SUCCESS) {
    putToDebug("IMU initialized.", 3);
  } else {
	putToDebug("Failed to initialize IMU.", 1);
    while (true);  
  }
  uint8_t readData = 0;
  if (myIMU.readRegister(&readData, KXTJ3_WHO_AM_I) == IMU_SUCCESS) {             // Get sensor the ID:
	  putToDebugWithNewline("KXTJ3 Addr: 0x"+String(readData), 3);
  } else {
	  putToDebug("******** KXTJ3 Communication error ********", 1);
  }
  if (myIMU.intConf(KXTJ3_THRESHOLD, KXTJ3_MOVEDUR, KXTJ3_NADUR, KXTJ3_POLARITY, KXTJ3_WURATE, KXTJ3_LATCHED, KXTJ3_PULSED, KXTJ3_MOTION, KXTJ3_DATAREADY, KXTJ3_INTPIN) == IMU_SUCCESS)  // TODO testing some move detection...
  {
    Serial.println("Latched interrupt configured.");
  } else {
    putToDebug("******** KXTJ3 Communication error motion detection ********", 1);
  }
}

void LP522DF_configureSensor(void) {
  lps25hb.begin(LPS25HB_ADDRESS);
	putToDebugWithNewline("LP522DF Addr: 0x"+String(lps25hb.whoAmI()), 3);
}

void HDC1080_configureSensor(void) {
  hdc1080.resetConfiguration();
  hdc1080.enableHeater();
  hdc1080.setHumidityResolution(GuL::HDC1080::HumidityMeasurementResolution::HUM_RES_14BIT);
  hdc1080.setTemperaturResolution(GuL::HDC1080::TemperatureMeasurementResolution::TEMP_RES_14BIT);
  hdc1080.setAcquisitionMode(GuL::HDC1080::AcquisitionModes::BOTH_CHANNEL);
  putToDebugWithNewline("HDC1080 Configured.", 3);
}

void KXTJ3_read(void) {			                                                 // read the accelerometer
  myIMU.standby(false);                                                      // Take IMU out of standby
  data_doc["x"] = int(myIMU.axisAccel(X) * 1000);                            // Write Acc. vals to JSON
  data_doc["y"] = int(myIMU.axisAccel(Y) * 1000);
  data_doc["z"] = int(myIMU.axisAccel(Z) * 1000);
  myIMU.standby(true);                                                        // Place IMU in standby
}

void HDC1080_read(void) {		// read the tempurature/humidity
  hdc1080.startAcquisition(GuL::HDC1080::Channel::BOTH);
  delay(hdc1080.getConversionTime(GuL::HDC1080::Channel::BOTH) / 1000);       // AND WAIT
  data_doc["t"] = hdc1080.getTemperature();                                   // Write HDC1008 temp to JSON 
  data_doc["h"] = hdc1080.getHumidity();                                      // Write HDC1008 humidity to JSON
}

void takeSaveSensorData(){                                                     // Take one full reading of all our Sensor data, pack it into the ringbuffer
  httpDataPayload = getOneSetSensorValues();                                   // Read I2C sesnors and load into JSON String  
  ringbuff.push(httpDataPayload);                                              // Updated JSON data_doc payload (read all sensors) & Push the JSON string into the ring buffer
  putToDebugWithNewline("\n*********************************",4);
  putToDebug("** Pushed to JSON RingBuffer ** \n",4);
  putToDebugWithNewline("* Entries:"+String(ringbuff.getCount()),4);
  putToDebugWithNewline("* Total Size:"+String(ringbuff.getTotalSize()),4);
  putToDebug("Ringbuffer Payload: "+httpDataPayload,4);                         // Display the added JSON object
  sensorSampleRateTimer = sensorSampleRate;                                     // Reset timer for data read --> RingBuffer
  putToDebugWithNewline("takeSaveSensorData() Pushed data to RingBuffer, next sample in:"+String(sensorSampleRate*TIMER1_MSP1)+" mS",2);
  putToDebugWithNewline("\n*********************************\n",4);
}
