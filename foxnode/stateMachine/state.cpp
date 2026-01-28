// =============================================================
// state.cpp
// =============================================================
/*
 * State machine to handle sharing of sensor values
 * from the sensor client to the drone server
 * for the "Sensor Dump to Server" state machine.
 */

#include "state.h"
#include "httpComms.h"
#include "sensor.h"
#include "eeprom.h"

// core communications variables to work with lower WiFi (things in httpComms.c)
unsigned int glob_requestConnctWiFi;
unsigned int glob_connectedToDrone;
unsigned int glob_requestDropWifi;
unsigned int glob_stateBable;
unsigned int glob_droneConnectJustDropped;

// means to de-couple dislay painting when done in API handlers
unsigned int wantToPaintDisplay;

// key variables parsed from the server response to a POST in the 'serv' data element
// These are parsed out in httpComms.c when a POST reply is received.
String glob_droneServ_srep = "NULL";  	      // server request action index is 'srep'
unsigned int glob_droneServer_sts; 	          // server current timestamp	index is 'sts'
time_t glob_droneServer_stsb = time(nullptr);	// the beginning timestamp for sensor readings,  index is 'stsb'
time_t glob_droneServer_stse = time(nullptr);	// the ending timestamp for sensor readings     index is 'stse'

unsigned int wifiSeesServerSSIDandJoins;      
unsigned int valuesShareState;                 
unsigned int glob_timer_droneReplyWait;       
unsigned int glob_timer_poll_delay;           
int rssi_current_value;                       
int wifiConnectState;                         
unsigned int glob_requestConnectWiFi;
unsigned int inBlindTime;
unsigned int wifiDroneBlindTimer;

// random nummber generatore code
unsigned int getRandomNumber(unsigned int minValue, unsigned int maxValue){
  // Ensure that minValue is less than or equal to maxValue
  if (minValue > maxValue) {
      unsigned int temp = minValue;
      minValue = maxValue;
      maxValue = temp;
  }

  // Generate random number within the range [minValue, maxValue]
  return rand() % (maxValue - minValue + 1) + minValue;
}

// resests the state machine, sets the default timeouts, negates the drop WiFi global, and
// asserts the "connect to the server if the signal is strong enought" to the WiFi Connection State machine.
void initializeSensorDump2Server(void){
	putToDebugWithNewline("initializeSensorDump2Server()...",3);
	valuesShareState = 0;
	glob_timer_droneReplyWait = POST_REPLY_WAIT;
	glob_timer_poll_delay = GLOB_TIMER_DEBUG_POLL_DELAY;
	rssi_current_value = -90;				                      // until we get a reading which happens in httpcomms.cpp httpState_WiFiConnect()
	glob_requestDropWifi = 0;		                          // we do not want to drop any connection
	glob_requestConnctWiFi = 0;		                        // tells the WiFi Connection machine we're ready to rumble
	glob_connectedToDrone = 0;
	glob_droneConnectJustDropped = 0;
	putToDebugWithNewline("initializeSensorDump2Server() Init compleate",3);
}

void initialize_WiConnectMachine(void){       // reset the wifi connect state machine.
	putToDebugWithNewline("initialize_WiConnectMachine()...",2);
	wifiConnectState = 0;
	glob_requestDropWifi = 0;		                  // we have no request to drop the WiFi connection
	glob_requestConnctWiFi = 1;		                // if we have good signal strength, we want to connect
}

unsigned int valuesShareMachine(void){        // AKA "MSM" Major State Machine 
	char s[384];
	String foo;
	unsigned long postStartTimeMs;			// the time in milliseconds we started this post process
	unsigned long pdt;						// how long it took to do the entire POST process 'p'ost 'd'elta 't'ime
	unsigned int bar;

	Serial.print("HTTP gate glob_connectedToDrone=");
	Serial.print(glob_connectedToDrone);
	Serial.print(" WiFi.status=");
	Serial.print(WiFi.status());
	Serial.print(" IP=");
	Serial.println(WiFi.localIP());

	if(wifiDroneBlindTimer != 0)return 1;					// we are locked out from communicatoins because we had a good sensor report.
	glob_stateBable = 1;                        // Verbosity of Statemachine logging serial output
	if(!wifiDroneBlindTimer && glob_droneConnectJustDropped){	          // If the connection to the drone is lost, the state machine is reset.
		if(valuesShareState){				// this is a one-shot message when the connection drops
			if(glob_stateBable){
				sprintf(s, "MSM Check: Connection to drone LOST");
				putToDebugWithNewline(s,2);
			}
		}
    glob_connectedToDrone = 0;
		glob_droneConnectJustDropped = 0;
		glob_timer_droneReplyWait = POST_REPLY_WAIT + 100;		      // start a 4 second timer
		valuesShareState = 0;
		return 5;
	}
	if(!wifiDroneBlindTimer && glob_connectedToDrone && rssi_current_value < (rssiConnectValue-30)){
		if(glob_stateBable && glob_timer_droneReplyWait == 0){
			sprintf(s, "MSM Check: rssi_current_value too low, dropping WiFi connection\n-glob_connectedToDrone: %d\nrssi_current_value: %d",glob_connectedToDrone,rssi_current_value);
			putToDebugWithNewline(s,2);
		}
		WiFiDropConnection();
		glob_timer_droneReplyWait = POST_REPLY_WAIT + 100;					// about 4 seconds
		valuesShareState = 0;
		return 5;
	}
	if(glob_connectedToDrone == 0 && glob_timer_droneReplyWait == 0){		// we're disconnect, try to re-connect
		glob_timer_droneReplyWait = POST_REPLY_WAIT + 100;
		putToDebugWithNewline("MSM Check: Attempting WiFi reconnect",2);
    putToDebugWithNewline("- glob_connectedToDrone: "+String(glob_connectedToDrone),4);
    putToDebugWithNewline("- glob_timer_droneReplyWait: "+String(glob_timer_droneReplyWait),4);
		//WiFi.reconnect();// DEBUG
		bool ok = ensureWifiConnected(getFoxNodeIP(thisFoxNodeId));


		if (ok && WiFi.status() == WL_CONNECTED && WiFi.localIP()[0] != 0) {
  		glob_connectedToDrone = 1;
  		wantToPaintDisplay = 1;
		} else {
  		glob_connectedToDrone = 0;
		}


		return 5;
	}
	// If we are here we are connected to the WiFi new, not in blind time, and have good signal strength
	postStartTimeMs = millis();
  switch(valuesShareState){
    case 0:		                                 // MSM 0: If not connected to WiFi try and connect
    	if(glob_connectedToDrone){
    		if(glob_stateBable && 2 <= serialLogLevel){
    				sprintf(s, "MSM %d  Idle: Have connection to WiFi Net, going to state 1", valuesShareState);
					putToDebugWithNewline(s,2);
				}
				valuesShareState = 1;
			}
			return 0;		                            // check every BigLoop cycle (10 mS)

		case 1:		                                // MSM 1: Sensor has just connected to drone WiFi network. Say "hi" to the drone server.
			if(glob_stateBable && 2 <= serialLogLevel){
				sprintf(s, "MSM %d  POST hi: POSTing 'core' JSON to drone, ccmd='hi'", valuesShareState);
				putToDebugWithNewline(s,2);
			}
			wifi_sendPost(createCoreData("hi")); // Send "hi" Fox-Node HTTP-POST to UAS
			if(glob_droneServ_srep =="sval"){
				valuesShareState = 5;              // process SVAR request 
				return 0;					// we want to process this request ASAP
			}
			if(glob_droneServ_srep == "conf"){
				valuesShareState = 15;             // process SLOG 
				return 0;					// we want to process this request ASAP
			}
			if(glob_droneServ_srep == "buby"){
				valuesShareState = 25;             // process BUBY
				return 0;					// we want to process this request ASAP
			}
			if(glob_droneServ_srep == "done"){
				valuesShareState = 35;             // process BUBY
				return 0;					// we want to process this request ASAP
			}
			// If we get here, we got a reply we don't know how to process.
			pdt = millis() - postStartTimeMs;
			if(glob_stateBable || serialLogLevel <= 1){
				sprintf(s,"MSM %d  got unknown server reply '%s', procTime=%d mS",valuesShareState, glob_droneServ_srep.c_str(), pdt);
				putToDebugWithNewline(s,2);
			}
			glob_timer_droneReplyWait = POST_REPLY_WAIT + 100;			 // wait for 4 seconds, then go back to state zero
			valuesShareState = 28;		// go back
			return 1;

		case 5:		                                                  // MSM 5: The drone server has requested that the sensor values be sent. (make an rval)
			if(glob_stateBable && 2 <= serialLogLevel){
				sprintf(s,"MSM %d  'hi' Post recived 'sval' reply: start TS=%d, end TS=%d", valuesShareState, gmtime(&glob_droneServer_stsb), gmtime(&glob_droneServer_stse));
				putToDebugWithNewline(s,2);
			}
			foo = CoreDataAndMerge("rval",glob_droneServer_stsb,glob_droneServer_stse);
			putToDebugWithNewline("MSM case 5: "+foo,1);
			wifi_sendPost(foo);
			
			if(glob_stateBable && 2 <= serialLogLevel){
				sprintf(s,"MSM %d  POSTing core 'rval' JSON and 'data' to drone", valuesShareState);
				putToDebugWithNewline(s,2);
			}
			valuesShareState = 6;                                      // Wait for RVAL UAS reply (BYE)
			// Fall through

		case 6:                                                     // MSM 6: Sent rval, what was UAS Server responce?
			// we have a reply from the drone server to the sensor client post of all the sensor values in the 'data' object
			pdt = millis() - postStartTimeMs;
			if(glob_stateBable && 2 <= serialLogLevel){
				sprintf(s,"MSM %d  Recived POST from sending data, got drone srep'='%s', procTime=%d",valuesShareState,glob_droneServ_srep.c_str(),pdt);
				putToDebugWithNewline(s,2);
			}
			if(glob_droneServ_srep =="sval"){   // NOTE: this will handle multiple SVAL UAS requests
				valuesShareState = 5;
				return 0;					// we want to process this request ASAP
			}
			if(glob_droneServ_srep =="conf"){
				valuesShareState = 15;
				return 0;					// we want to process this request ASAP
			}
			if(glob_droneServ_srep =="buby"){
				valuesShareState = 25;
				return 0;					// we want to process this request ASAP
			}
			if(glob_droneServ_srep == "done"){		// sever is disconnecting now.
				valuesShareState = 35;
				return 0;					// we want to process this request ASAP
			}
			// If we get here, we got a reply we don't know how to process.
			if(glob_stateBable || serialLogLevel <= 1){
				sprintf(s,"MSM %d  *got unknown server reply '%s'",valuesShareState, glob_droneServ_srep.c_str());
				putToDebugWithNewline(s,1);
			}
			glob_timer_droneReplyWait = POST_REPLY_WAIT + 100;			 // wait for 4 seconds, then go back to state zero
			valuesShareState = 28;		// go back
			return 1;

		case 15:		                                               // MSM 15: drone sever sent a conf ('conf'igure) request
			if(glob_stateBable && 2 <= serialLogLevel){
				sprintf(s,"MSM %d  'hi' POST received 'conf' reply", valuesShareState);
				putToDebugWithNewline(s,2);
        
			}
//			createCoreData("slog");		// create the core data object in the POST buffer
//			createCoreData("ldat");		// append the sensor object "data" to the POST buffer. This subber also does
												// bounds check on start and stop times provided vs. what is available.
			glob_timer_droneReplyWait = POST_REPLY_WAIT;	// give the drone 3 seconds (300 * .01) to reply
			glob_timer_poll_delay = GLOB_TIMER_DEBUG_POLL_DELAY;
			if(glob_stateBable && 2 <= serialLogLevel){
				sprintf(s,"MSM %d  POST srep: POSTing core 'slog' JSON and'ldat' to drone", valuesShareState);
				putToDebugWithNewline(s,2);
			}
			valuesShareState = 16;     // Wait for UAS reply from SLOG
			return 2;

		case 16:		                                                // MSM 16: we have a reply back from sending the (CONF)
			// we have a reply from the drone server to the sensor client post of all the log files in the 'ldat' object
			pdt = millis() - postStartTimeMs;
			state_processSCRVReply();			// process the post reply and set globals from JSON data elements
			if(glob_stateBable){
				sprintf(s,"MSM %d  have POST 'rval' reply: got drone srep '%s', procTime=%d", valuesShareState,glob_droneServ_srep.c_str(),pdt);
				putToDebugWithNewline(s,2);
			}
			if(glob_droneServ_srep == "sval"){		// sever wants sensor values
				valuesShareState = 5;
				return 0;					// we want to process this request ASAP
			}
			if(glob_droneServ_srep == "conf"){		// server wants log files
				valuesShareState = 15;
				return 0;					// we want to process this request ASAP
			}
			if(glob_droneServ_srep == "buby"){		// sever is ready to disconnect
				valuesShareState = 25;
				return 0;					// we want to process this request ASAP
			}
			if(glob_droneServ_srep == "done"){		// sever is disconnecting now.
				valuesShareState = 35;
				return 0;					// we want to process this request ASAP
			}
			// If we get here, we got a reply we don't know how to process.
			if(glob_stateBable || serialLogLevel <= 1){
				sprintf(s,"MSM %d  *got unknown server reply '%s'",valuesShareState,glob_droneServ_srep.c_str());
				putToDebugWithNewline(s,1);
			}
			glob_timer_droneReplyWait = POST_REPLY_WAIT + 100;			 // wait for 4 seconds, then go back to state zero
			valuesShareState = 28;		// go back
			return 1;

		case 25:		                                                // MSM 25: Drone sever sent a buby. It's done with it's transactions with the drone server.
			if(glob_stateBable && 2 <= serialLogLevel){
				sprintf(s,"MSM %d  Recieved POST reply of 'buby', replying 'bye' to UAS Server", valuesShareState);
				putToDebugWithNewline(s,2);
			}
			wifi_sendPost(createCoreData("bye"));		// create the core data object in the POST buffer
			valuesShareState = 26;                  // wait for UAS (DONE)
      // Fall thorugh 
		case 26:		                                                // MSM 26: We have a replly back from sending the 'bye' to the drone sever
			pdt = millis() - postStartTimeMs;
			if(glob_stateBable){
				sprintf(s,"MSM %d  Recived 'bye' reply: got drone srep '%s', procTime=%d \n** Transaction completed **\n",valuesShareState,glob_droneServ_srep.c_str(),pdt);
				putToDebugWithNewline(s,2);
			}
			if(glob_droneServ_srep == "sval"){		// sever wants sensor values
				valuesShareState = 5;
				return 0;					// we want to process this request ASAP
			}
			if(glob_droneServ_srep == "conf"){		// server wants log files
				valuesShareState = 15;
				return 0;					// we want to process this request ASAP
			}
			if(glob_droneServ_srep =="buby"){		// sever is ready to disconnect
				valuesShareState = 25;
				return 0;					// we want to process this request ASAP
			}
			if(glob_droneServ_srep == "done"){		// sever is disconnecting now.
				valuesShareState = 35;
				return 0;					// we want to process this request ASAP
			}
			// If we get here, we got a reply we don't know how to process.
			if(glob_stateBable || serialLogLevel <= 1){
				sprintf(s,"MSM %d  *got unknown server reply '%s'",valuesShareState,glob_droneServ_srep.c_str());
				putToDebugWithNewline(s,1);
			}
			glob_timer_droneReplyWait = POST_REPLY_WAIT + 100;			 // wait for 4 seconds, then go back to state zero
			valuesShareState = 28;		// go back
			return 1;

		case 28:                                                   // MSM 28: Catch all State
			putToDebugWithNewline("MSM 28: ",2);
      if(glob_timer_droneReplyWait)return 1;
			valuesShareState = 0;
			return 1;

			// we get  to case 35 becuase the drone server replied with a "done"
			// In general, case 35 disconnects from the WiFi, waits for TIMER_DROPED_WIFI_BLINDTIME (long timeout, 15 to 60 seconds),
			//   then re-connects to the drone server WiFi, and finally resets this state machine so when the sensor client sees a
			//   drone server WiFi network with the correct SSID it can start the "hi" procces to send sensor values all over again.
		case 35:		                                              // MSM 35: The client POSTed a bye, the server replied with a 'done'. Our work here is done.
			if(glob_stateBable && 2 <= serialLogLevel){
				printf(s, "MSM 35: UAS reply from Post was 'done', Dropping WiFi connection, procTime=%d", pdt);
				putToDebugWithNewline(s, 2);
        putToDebugWithNewline("MSM 35: Going Blind",2);
			}
			inBlindTime = 1;                // turn off entry check for connectivity / signal strength at the top of this subroutine
			wifiDroneBlindTimer = blindTimeDelay;		// wait about 15 seconds
      valuesShareState = 36;
      //return 100;
      // Fall through

		case 36:                                                  // MSM 36: We are waiting for the end of a transmit session blind timer to expire
			putToDebugWithNewline("MSM 36: wifiDroneBlindTimer: "+String(wifiDroneBlindTimer),2);
      if(wifiDroneBlindTimer)return 1;
			putToDebugWithNewline("MSM 36: Blind time expired, back to talking to Drone Server",2);
			valuesShareState = 0;
      inBlindTime = 0;
			return 1;
  }
  sprintf(s,"MSM %d  * Invalid Valueshare State Machine case.", valuesShareState);      // MSM: unknown case
  putToDebugWithNewline(s,1);
  glob_timer_droneReplyWait = POST_REPLY_WAIT + 100;			 // wait for 4 seconds, then go back to state zero
  valuesShareState = 28;		// go back
  return 1;
}