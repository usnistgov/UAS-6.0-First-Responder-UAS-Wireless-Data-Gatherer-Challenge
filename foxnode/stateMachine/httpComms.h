#ifndef HTTPCOMMS_H
#define HTTPCOMMS_H

// SCOPE: Support system Network communications, WiFi/HTTP

// Includes
#include "serial.h"
#include "sensor.h"
#include "RingBuffer.h"                                 // for HTTPClient

// Global Variables
//// UAS Server Target vars
#define WIFI_SSID     "uas6"					// target network for  UAS 6.0 Prize challenge
#define WIFI_PASSWORD "hello123"			// target network for  UAS 6.0 Prize challenge
#define UAS_Server_IP "192.168.40.20" // target Drone Server IP address
#define UAS_Server "http://" UAS_Server_IP	// target Drone Server URI

#define UAS_Gateway "192,168,40,20"			// Gateway for Sensor Client
#define UAS_DNS		  "8,8,8,8"				    // DNS for Wifi, in case it needs it
#define UAS_Subnet	"255,255,0,0"

// for loating up the Real Time Clock  RTC  from a hotspot on the net
#define NTP_WIFI_SSID     "UAS_NTP"				// target network for  NTP (external WiFi Hotspot-system setup only)
#define NTP_WIFI_PASSWORD "whatTime!"			// target network for  NTP

//// timeout values and variables for the HTTP state machine
#define WIFI_DROP_DEAD_WAIT 3000/TIMER1_MSP1;				     // if we loos WiFi, presumably due to loss of signal, don't try to connect
#define GLOB_DROP_CONNECT_DEAD_TIME 5000/TIMER1_MSP1;	   // Minimum dead time inmposed at HTTP level. Note that the MSM / state machine

extern WebServer webServer;                              // global webServer for HTTP msg's 
extern int httpResponseCode;
extern unsigned int wifiDropDeadTimer;
extern IPAddress FN_IP;

// Function Declarations
void initializeHttpMsmValues(void);
IPAddress getFoxNodeIP(unsigned short thisFoxNodeId);
void setup_webserver_routing(void);
void initializeHTTP(void);
void httpState_WiFiConnect(void);
void wifi_sendPost(const String& payload);	             // compatibility with MSM
void sendHttpPost(const String& payload);
//TODO this used to be our WiFi POST interrupt function//unsigned int httpCheckForPostReply(void);
unsigned int httpConnectedToWiFi(void);
void setup_routing(void);
void handleNotFound(void);
void WiFiDrpConnection(void);
void http_appendCoreJson(void);				                   // MSM calls, create and put the "core" data object into the HTTP POST buffer
void http_appendDataJson(void);				                   // MSM calls, create and put the "data" data object into the HTTP POST buffer
void state_processSCRVReply(void);			                 // MSM calls once a POST reply is recieved
// lowest level WiFi stuff
void WiFiDropConnection(void);
void WiFiInit(IPAddress FN_IP);

#endif    // HTTPCOMMS_H
