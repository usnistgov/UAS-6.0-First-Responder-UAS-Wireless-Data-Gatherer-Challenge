// HTTP ESP32 specific interface routines
#include "httpComms.h"
#include "state.h"
#include "serial.h"
#include "timer.h"
#include "display.h"
#include "sensor.h"						// for Wire (SPI)
#include "RingBuffer.h"					// for Ringbuffer class "ringbuff"
#include "eeprom.h"

WebServer webServer(80);				// initialize Server w/ def HTTP port assignment
IPAddress netmask(255,255,0,0);			// Entire UAS echosystem is a 16 bit nework
IPAddress clientIpAddr(192,168,40,30);	// will re-define this when we have a FoxNode Sensor Client ID
IPAddress serverIpAddr(192,168,40,20);	// the fixed IP address of the Drone Server
IPAddress dnsServer(8,8,8,8);			// if client wants to reach out over the internet (unlikely)

int httpResponseCode;					// things like 200 and 400. If that doen't make sense,
										          // you should not be messing with this software.
unsigned int httpWiFiState;
unsigned int wifiDropDeadTimer;			// loss of signal to try to connect again dead time
unsigned int WiFiHaveDhcpReply;			// true if have had a good DNS reply so the client has an IP address

void initializeHttpMsmValues(void) {	// Initialize the WiFi connect state monitor, used to create clean logging for WiFi conectivilty events
	httpWiFiState = 0;
	WiFiHaveDhcpReply = 0;				// only called once on boot
}

IPAddress getFoxNodeIP(unsigned short thisFoxNodeId){		// formulate IP address based on Fox-Node ID
	if(thisFoxNodeId > 60)thisFoxNodeId = 0;				// safety
	return IPAddress(192, 168, 40, thisFoxNodeId + 30);
}

////////  next 3 subroutines are tied to Wifi events
// IRQ like event where the sesnor client has connected to the Drone Server WiFi net.
// In theory, this would be just seeing and connecting to the Wifi network, possibly before DHCP negotiation
void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {          // so that the state machine knows if the foxnod is connected to the server drone or not.
}

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
  wantToPaintDisplay = 1;			// (BLUE) display connection info
  glob_connectedToDrone = 1;  // STATE MACHINE FLAG: Unlock Sensor Dump to Server given UAS connection
}


void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
	/*
  putToDebugWithNewline("WiFiStationDisconnected() WiFi Network Disconnect", 2);
		if(serialLogLevel >= 3){
		putToDebug("WiFi Station: ", 3);
		putToDebug(String(event), 3);
		putToDebug(" Reason: ", 3);
		putToDebugWithNewline(String(info.wifi_sta_disconnected.reason), 3);
	}
  */
	WiFi.disconnect(true);
	//glob_connectedToDrone = 0;                                         // STATE MACHINE FLAG: lock Sensor Dump to Server given no UAS // NOTE: that glob_connectedToDrone is an int, but used as a bool.
	glob_droneConnectJustDropped = 1;                                  // Update Global to show SMS that we are in a WiFi dissconnection state
	wantToPaintDisplay = 2;			// (ORANGE) display disconnection message
}

//This subroutine attaches event handlers to subroutines for key WiFi evernts.
void WiFiInit(IPAddress foxNodeIp){
	unsigned long timeout;
	unsigned long startMillis;
	char s[128];
  
  putToDebugWithNewline("WiFiInit() Starting WiFi", 2);
  
	WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);		      // Implement WiFi Callback functions to avoid handles in main loop
	WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);					              // _guessing_ this is when client has IP from DHCP ?
	WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);	  // Handle WiFi reconnect //TODO check this... might be fighting with WiFi SM ...
	
  // cofig arguments are: IP address, Gateway, Subnet, DNS
	WiFi.mode(WIFI_STA);	
  WiFi.config(foxNodeIp, IPAddress(UAS_Gateway), IPAddress(UAS_Subnet),IPAddress(UAS_DNS));

  putToDebugWithNewline("WiFiInit() Looking for SSID: "+String(WIFI_SSID),2);
  putToDebugWithNewline("WiFiInit()    with Password: "+String(WIFI_PASSWORD),2);
  putToDebugWithNewline("WiFiInit(): calling WiFi.begin", 3);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  delay(1000);

  putToDebugWithNewline("- glob_connectedToDrone: "+String(glob_connectedToDrone),4);
  putToDebugWithNewline("- wantToPaintDisplay: "+String(wantToPaintDisplay),4);
  WiFi.setTxPower(WIFI_POWER_7dBm);                                // Set Tx power low (for Stage 2 this setting works well) NOTE: power def --> https://github.com/espressif/arduino-esp32/blob/70786dc5fa51601f525496d0f92a220c917b4ad9/libraries/WiFi/src/WiFiGeneric.h#L47
  putToDebugWithNewline("WiFiStationConnected () - WiFi-STA Tx Power: "+String(WiFi.getTxPower()), 2); // Check power setting and send to Serial
  putToDebugWithNewline("WiFiInit(): Exiting\n",2);
	// note that our IRQ service routines will print if there was a connection here.
	// the routines WiFiStationConnected() on WiFi net connect and WiFiGotIP() if we were expecting to get a DHCP address
	// The glboal used in the MSM 'glob_connectedToDrone' is also set to non zero in either of these routines.
}

void WiFiDropConnection(void){                                       // this subroutine is called when the state machine wants to drop the connection (if it exists) for a "back off" period 
	putToDebugWithNewline("WiFiDropConnection()", 3);
  if(httpConnectedToWiFi()){
		WiFi.disconnect(true);			                                     // TODO do we know if we get an error if you try to disconnect
		//glob_connectedToDrone = 0;
		delay(1000);
  }					
}

void WiFiReconnect(void){
	putToDebugWithNewline("WiFiReconnect()", 4);
	if(httpConnectedToWiFi() == 0){
	WiFi.reconnect();
	glob_droneConnectJustDropped = 0;                                 // Update Global to show SMS that we are in a WiFi connected state
	}
}

void setup_webserver_routing(void){
	webServer.onNotFound(handleNotFound);                                                     // handle 404
	webServer.begin();
	Serial.println("setup_webserver_routing() done");
}

unsigned int httpConnectedToWiFi(void){
  return glob_connectedToDrone;

  if (WiFi.status() != WL_CONNECTED) {
		//glob_connectedToDrone = 0;				                             // this var will tell SM we lost connectivity.
		return 0;
	}
	return 1;
}

void handleNotFound(void) {
	String message = "File Not Found\n\n";
	message += "URI: ";
	message += webServer.uri();
	message += "\nMethod: ";
	message += (webServer.method() == HTTP_GET) ? "GET" : "POST";
	message += "\nArguments: ";
	message += webServer.args();
	message += "\n";
	for (uint8_t i = 0; i < webServer.args(); i++) {
		message += " " + webServer.argName(i) + ": " + webServer.arg(i) + "\n";
	}
	webServer.send(404, "text/plain", message);
}

// this has evelved into just a logging mechanizm for the state of the wifi connection 21mar2025 PDH

void httpState_WiFiConnect(void){                                  // This is a state machine that deals with detecting/knowing if the foxnode client is connect to the drone sever.
  // This provides the functionaltiy to try to get connected, and to be able to know when an "edge event"
  // has happpend, auch as a transition from not connected to the drone WiFi to being connected.
    char s[64];  // Dynamically allocate scrach memory char * to String conv....
    rssi_current_value = WiFi.RSSI();		// constantly update our receive RSSI
    switch(httpWiFiState){
      case 0:		// we are not connected, try to connect if requested
          // don't connect until there is a request to connect from the Main State Machine  (MSM)
          if(httpConnectedToWiFi() == 0)return;
          putToDebugWithNewline("HTTP SM 0: We are connected to WiFi",3);
          httpWiFiState = 1;
          return;
      case 1:		// If we are here, we are connected to WiFi
          if(glob_connectedToDrone == 0){                           // Either the WiFi droped (from IRQ handler WiFiStationDisconnected() )
              putToDebugWithNewline("HTTP SM 1: Connection to Drone dropped",3);
              httpWiFiState = 3;
              wifiDropDeadTimer = WIFI_DROP_DEAD_WAIT;                             // set in httpComms.h, typicall 3 seconds
              return;
          }
          return;
      case 3:		// the MSM requested we drop the wifi connection, or the WiFi signal got too weak.
		// Now we go blind for WIFI_DROP_DEAD_WAIT, typically 3 seconds.
		// note that the MSM will impost a longer delay time after
		// requestiong a WiFi disconnect of about 1 minute. Another timeout here is a "safety"
		// to keep things from getting crazy if the MSM is behaving badly
		if(wifiDropDeadTimer)return;
		putToDebugWithNewline("HTTP SM 3: Drone wait time over, resume monitoring WiFi state",2);
		httpWiFiState = 0;                                  // After the safety time has elapsed, we go back to waiting
		return;                                                                                                 // to connect to the server.
    }
    // If we fall through, we have a bad state value
    sprintf(s,"* State %d is out of range- fell through the switch, resetting to zero", httpWiFiState);
    putToDebug(s, 1);
    httpWiFiState = 0;
}

void wifi_sendPost(const String& payload){		                     // Compatibility with MSM link independent view of the world
	sendHttpPost(payload);
}

void sendHttpPost(const String& payload) {                         // The payload for the HTTP post has been created and can be sent to the Server
	if(httpConnectedToWiFi()) {
		HTTPClient http;
		http.begin(UAS_Server);
		http.addHeader("Content-Type", "application/json");
		putToDebugWithNewline("SEND HTTP POST:"+String(payload), 4);
		httpResponseCode = http.POST(payload);

		putToDebug("HTTP Response code: ",4);
		putToDebugWithNewline(String(httpResponseCode),4);
		if(httpResponseCode == 200) {
			String responseJSON = http.getString();
			putToDebug("sendHttpPost() Server POST responseJSON: ",4);
			putToDebugWithNewline(String(responseJSON),4);putToDebugWithNewline("",4);
			deserializeJson(postRxPayload_doc, responseJSON);		// Write JSON general HTTP-POST payload into JSON POST_doc for API processing
			state_processSCRVReply();								// Process UAS reply based on SVAL

		}else{														// else If POST request fails, log the error
			putToDebug("Error: ",1);
			putToDebugWithNewline(String(httpResponseCode),1);putToDebugWithNewline("",1);
		}
		http.end();
	}else{
		putToDebugWithNewline("sendHttpPost(): WiFi not connected! **POST Failed**",1);
	}
}

void state_processSCRVReply(void){					                       // process the HTTP POST reply from the Drone Server then pass into SM
	putToDebugWithNewline("\n*** STATE PROCESS SCRVR REPLY ***",4);

	String srep = postRxPayload_doc["serv"]["srep"];						// Read "srep" to use in if-else API state machine
	glob_droneServ_srep = srep;
	putToDebugWithNewline("SREP FOUND: "+srep,4);

	if (srep == "sval") {         // UAS requests data over Delta-T (stse - stsb)		
		glob_droneServer_stsb = postRxPayload_doc["serv"]["stsb"];			// Beginning timestamp for data payload pull
		glob_droneServer_stse = postRxPayload_doc["serv"]["stse"];			// Ending timestamp for data payload pull
		putToDebug("Sending all data from End Time: ", 4);
		putToDebug(ctime(&glob_droneServer_stsb), 4);						// Display Ringbuffer time datum for data pull
		putToDebug("To Start Time: ",4);
		putToDebugWithNewline(ctime(&glob_droneServer_stse), 4);			// Display Ringbuffer time datum for data pull
		}
	else if (srep == "ping") {    // UAS request the core data object againi)
		// no variables to extract from 'serv' object
	}
	else if (srep == "slog") {    // UAS requests all log objects stored
		// no variables to extract from 'serv' object
	}
	else if (srep == "buby") {    // UAS requests FN disconnect for a time
		// no variables to extract from 'serv' object
	}
	else if (srep == "done") {    // UAS requests FN disconnect for a time
		// no variables to extract from 'serv' object
	}
	else {
		putToDebugWithNewline("state_processSCRVReply() Unknown SREP cmd '"+glob_droneServ_srep+"'", 4);
	}
	tft_display(4);  // (BLACK) update display to signal HTTP Rx
}

const char* wl_status_to_string(wl_status_t status) {

  switch (status) {
    case WL_NO_SHIELD: return "WL_NO_SHIELD";
    case WL_IDLE_STATUS: return "IDLE";
    case WL_NO_SSID_AVAIL: return "WL_NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED: return "WL_SCAN_COMPLETED";
    case WL_CONNECTED: return "WL_CONNECTED";
    case WL_CONNECT_FAILED: return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST: return "WL_CONNECTION_LOST";
    case WL_DISCONNECTED: return "WL_DISCONNECTED";
  }
}
// Mary had a an API
// with definitions neat
// but when the drone got sensor values
// it really was quite sweet

