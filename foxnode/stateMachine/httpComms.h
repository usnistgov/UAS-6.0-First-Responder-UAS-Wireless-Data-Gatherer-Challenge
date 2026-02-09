// =============================================================
// httpComms.h
// =============================================================
#include <Arduino.h>   // brings in uint32_t (and a lot of Arduino core types)
#include <WiFi.h>      // brings in IPAddress and WiFi (ESP32/ESP8266 core)
#include "secrets.h"   // Support for client certs and wifi secrets

#ifndef HTTPCOMMS_H
#define HTTPCOMMS_H
bool ensureWifiConnected(IPAddress fallbackIp);

// SCOPE: Support system Network communications, WiFi/HTTP, adjust dhcp/static timeout here
bool connectWiFi_DHCP_thenStaticEveryAttempt(WifiProfile profile,
                                             IPAddress fallbackIp,
                                             uint32_t dhcpTimeoutMs = 3000,
                                             uint32_t staticTimeoutMs = 5000);

// Includes
#include "serial.h"
#include "sensor.h"
#include "RingBuffer.h"                                 // for HTTPClient

// Global Variables
// WiFi SSIDs and PSKs are stored in NVS via secrets.h
//// UAS Server Target vars
#define UAS_Server_IP "192.168.40.20:8443" // target Drone Server/Data Ferry IP address
#define UAS_Server "https://" UAS_Server_IP	// target Drone Server URI

#define UAS_Gateway "192.168.40.20"			// Gateway for Sensor Client
#define UAS_DNS		  "8.8.8.8"				    // DNS for Wifi, in case it needs it
#define UAS_Subnet	"255.255.0.0"

//// timeout values and variables for the HTTP state machine
#define WIFI_DROP_DEAD_WAIT 3000/TIMER1_MSP1;				     // if we loos WiFi, presumably due to loss of signal, don't try to connect
#define GLOB_DROP_CONNECT_DEAD_TIME 5000/TIMER1_MSP1;	   // Minimum dead time inmposed at HTTP level. Note that the MSM / state machine

extern WebServer webServer;                              // global webServer for HTTP msg's 
extern int httpResponseCode;

// TLS status for on-device display/debug (no secrets printed)
// 0 = N/A (no HTTPS attempt yet), 1 = OK, 2 = FAIL
#define TLS_STATE_NA   0
#define TLS_STATE_OK   1
#define TLS_STATE_FAIL 2
extern volatile uint8_t tlsState;
extern char tlsLastError[48];

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