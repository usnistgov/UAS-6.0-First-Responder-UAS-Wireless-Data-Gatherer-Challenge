// =============================================================
// state.h
// =============================================================
#ifndef STATE_H
#define STATE_H

// SCOPE:

// Includes
#include "sensor.h"
#include <cstring>                                        // lib for strcmp

// Global Variables
//// START Fox-Node specific vars                                         // Unique ID for the Fox-Node NOTE: this var will update the base FN-IP address (192.168.1.(30 + thisFoxNodeId)
#define DRONE_SERVER_IP_ADDRESS_STRING IPAddress(192,168,40,20)

#define CLOCK_MS_TICK_RATE 1							// number of milliseconds per timer tick
#define TIMER1_MSP1 10									// number of milliseconds per timer IRQ in "bigloop", typically 1 mS

//// Key time delays. Assuming a 10 mS big loop time.
#define SENSOR_SAMPLE_RATE 30000/TIMER1_MSP1              // 1 minute per sensor set sample
#define POST_REPLY_WAIT 4000/TIMER1_MSP1                  // 4 seconds to reply to a POST
#define GLOB_TIMER_DEBUG_POLL_DELAY 500/TIMER1_MSP1		  // Throttle delay for log files when in a polling loop. 500 mS
#define TIMER_DROPED_WIFI_BLINDTIME 15000/TIMER1_MSP1     // How long we go blind after WiFi connection drops. 5 seconds 
#define GLOB_TIMER_WIFI_RSSI_STRENGTH 10                  // units are [dBm]
#define GLOB_TIMER_WIFI_WRONG_SSID_PAUSE 10/TIMER1_MSP1   // If we see the wrong SSID, we wait 5 seconds
#define RSII_DROP_VALUE 10

//// key "decrement and stick at zero" timer variables
extern unsigned int glob_timer_droneReplyWait;		        // timer for waiting for a POST reply from the sever
extern unsigned int glob_timer_poll_delay;			          // timer used to Throttle delay for log files when in a polling loop.
//// Core state machine variables
extern unsigned int valuesShareState;		                  // current state of the "Sensor Dump to Server" state machine
extern unsigned int glob_stateBable;		                  // if non-zero, the log file/serial debug port shows state machine progress
//// key variables parsed from the server response to a POST in the 'serv' data element
extern String glob_droneServ_srep;	                      // server request action						index is 'srep'
extern unsigned int glob_droneServer_sts; 	              // server current timestamp						index is sts
extern time_t glob_droneServer_stsb;	                    // the beginning timestamp for sensor readings,  index is 'stsb'
extern time_t glob_droneServer_stse;	                    // the ending timestamp for sensor readings     index is 'stse'
//// key globals that allow the state machines to intereact with each other
extern unsigned int glob_requestConnectWiFi;	              // if the drone sever is near and signal strong enough, go ahead and
											// join the WiFi network. (when conencted, the glob_connectedToDrone is set)
											// set by Sensor Dump to Sever state machine when it's ready to go and
											// after a good connection session with the server, it wants to "go dark"
											// for a minute or so. When zero, it stops the WiFi connection state machine
											// from connection to the drone server.
extern unsigned int glob_droneConnectJustDropped;
extern unsigned int glob_connectedToDrone;	              // if non-zero, the lower IP stacks are connected to the drone WiFi net
extern unsigned int glob_requestConnectWiFi;							// set by WiFi Connection state machine
extern unsigned int glob_requestDropWifi;	// if non-zero, the client need to drop off the sever WiFi net.
											// The underlying system can then clear this variable when client is off.
											// set by Sensor Dump to Sever state machine when a communications session is done
											// set by the Sensor Dump to Server state machine
											// set when the ESP32 HTTP stacks get a 200 response
extern int rssi_current_value;                            // TODO 
extern int wifiConnectState;                              // TODO 
extern unsigned int wifiSeesServerSSIDandJoins;           // TODO check this var
extern unsigned int wifiDroneBlindTimer;

// mechanizm for posting a request to refesh the TFT display
extern unsigned int wantToPaingDisplay;

// Function Declarations
void initializeSensorDump2Server();
unsigned int getRandomNumber(unsigned int minValue, unsigned int maxValue);
void initialize_WiConnectMachine(void);
unsigned int valuesShareMachine();

#endif  // STATE_H