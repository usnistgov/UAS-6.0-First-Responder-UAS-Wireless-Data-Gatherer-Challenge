// NIST-PSCR
// UAS 6.0 Prize Challenge "Fox-Node" client source code
// =============================================================
// stateMachine.ino
// =============================================================
#include "state.h"
#include "sensor.h"
#include "httpComms.h"
#include "RingBuffer.h"
#include "serial.h"
#include "timer.h"
#include "sensor.h"
#include "display.h"
#include "eeprom.h"
#include <esp_system.h>
#include "secrets.h"

void setup() {
  initializeSerial();                 
  putSetSerialLogLevel(1);                // Set Verbosity of log level 
  secretsInit();
  WiFi.disconnect(true);                  // init setp, this clears the WiFi object
  eepromSetVariablesToDefault();          // set this foxnod specifc defaults, will be changed when values read in from EEPROM
  
  tft_display(0);                         // Onboard TFT display for ESP32 display, also inits wantToPaintDisplay variable
  initTimers();                           // Timer ISR that run statemachine's 
  if(1){                                  // used to skip getting NPT from internet for debugging
    if(initNTP() != 0){ 
        putToDebugWithNewline("Error with RTC init",1);   // Setup I2C connected RTC via NTP or handle lack of NTP/RTC
    } else {
      
    }
  }
  eepromXferRtcToVariables();            // Read core vars for personality (FN-ID, GPS, sensor rate)
  putToSerialWithNewline("**Fox-Node Init**");
  putToSerialWithNewline("FoxNode ID: "+String(thisFoxNodeId));
  putToSerialWithNewline("**I2C**");    
  initializeSensors();                    // I2C Sensor setup
  InitDummyData();                        // setup httpDataPayload for ringbuffer sizing 
  initializeHttpMsmValues();              // initialize key interface timers to state machine
  setup_webserver_routing();              // Start FN-API
  IPAddress thisFoxNodeIP = getFoxNodeIP(thisFoxNodeId);      // grab the defauilt FoxNode ID based on the FoxNode ID number , Dynamic calculation based on thisFoxNodeId to set static IP
  ringBufferInit();
  initializeHttpMsmValues();              // initialize WiFi state variables in httpComms.cpp
  initializeSensorDump2Server();          // Set vars for state.cpp 
  WiFiInit(thisFoxNodeIP);                // turn on WiFi, try to connect to Drone Server  
}

void loop() {                 // BIG LOOP this runs all of our statemachines
  unsigned int safteyCount;                                                           
  webServer.handleClient();               // do this in main loop for API handling 
  if (Timer1_flag) {                      // Timer1 == 10 second ISR 
    if(sensorSampleRateTimer == 0){       // Command Ringbuffer data ...  
      takeSaveSensorData();
    }
    httpState_WiFiConnect();
    safteyCount = 4; 
    while(safteyCount-- > 0 && valuesShareMachine() == 0);    // only allow 4 state machine calls in quick sucssesion --> valuesShareMachine
    if(wantToPaintDisplay)tft_display(wantToPaintDisplay);    // had queded request to re-paint the TFT display (probaqbly from an inteerupt handerl)
  }
  delay(10);                              // allow the cpu to switch to other tasks
}