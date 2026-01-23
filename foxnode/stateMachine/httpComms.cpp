// HTTP ESP32 specific interface routines
#include "httpComms.h"
#include "state.h"
#include "serial.h"
#include "timer.h"
#include "display.h"
#include "sensor.h"						          // For Wire (SPI)
#include "RingBuffer.h"					        // For Ringbuffer class "ringbuff"
#include "eeprom.h"
#include "WiFi.h"

//The following block sets the FoxNode for DHCP instead of Static IP
//To set back to Static IP, comment this section and edit commented sections below
bool connectWiFi_DHCP(const char* ssid, const char* pass, uint32_t timeoutMs = 15000) {
  WiFi.persistent(false);     // avoid flash wear from saving creds repeatedly
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  // Ensure we are not carrying an old static config
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE); // clears static config on ESP32

  WiFi.begin(ssid, pass);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
    delay(250);
  }

  return (WiFi.status() == WL_CONNECTED);
}

WebServer webServer(80);				        // Initialize Server w/ def HTTP port assignment
IPAddress netmask(255,255,0,0);			    // Entire UAS echosystem is a 16 bit classless nework
IPAddress clientIpAddr(192,168,40,30);	// Base IP for FoxNode IP range, IPs actually start at 80, as defined below (Don't change this)
IPAddress serverIpAddr(192,168,40,20);	// The fixed IP address of the Drone Server
IPAddress dnsServer(8,8,8,8);			      // If client wants to reach out over the internet

int httpResponseCode;					// Standard HTTP responses like 200 and 400.
unsigned int httpWiFiState;
unsigned int wifiDropDeadTimer;			// Loss of signal to try to connect again or "dead time"
unsigned int WiFiHaveDhcpReply;			// True if have had a good DNS reply so the client has an IP address

void initializeHttpMsmValues(void) {	// Initialize the WiFi connect state monitor, used to create clean logging for WiFi conectivilty events
	httpWiFiState = 0;
	WiFiHaveDhcpReply = 0;				// Only called once on boot
}

IPAddress getFoxNodeIP(unsigned short thisFoxNodeId){		// Formulate IP address based on Fox-Node ID
	if(thisFoxNodeId > 60)thisFoxNodeId = 0;				      // Logic for checking FoxNode ID, Don't assign over 175.
	return IPAddress(192, 168, 40, thisFoxNodeId + 80);   // Add 80 to FoxNode ID to keep it out of lower ranges for static devices
}

////////  next subroutine are tied to Wifi events
// IRQ like event where the sesnor client has connected to the Drone Server WiFi net.
// In theory, this would be just seeing and connecting to the Wifi network, possibly before DHCP negotiation

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
  wantToPaintDisplay = 1;			// (BLUE) display connection info
  glob_connectedToDrone = 1;  // STATE MACHINE FLAG: Unlock Sensor Dump to Server given UAS connection
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {

	WiFi.disconnect(true);
	glob_connectedToDrone = 0;                                       // STATE MACHINE FLAG: lock Sensor Dump to Server given no UAS // NOTE: that glob_connectedToDrone is an int, but used as a bool.
	glob_droneConnectJustDropped = 1;                                  // Update Global to show SMS that we are in a WiFi dissconnection state
	wantToPaintDisplay = 2;			                                       // (ORANGE) display disconnection message
}
//This subroutine attaches event handlers to subroutines for key WiFi evernts.
void WiFiInit(IPAddress foxNodeIp){

  putToDebugWithNewline("WiFiInit() Starting WiFi", 2);
  
	WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);		      // Implement WiFi Callback functions to avoid handles in main loop
	WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);	  // Handle WiFi reconnect //TODO check this... might be fighting with WiFi SM ...

  // cofig arguments are: IP address, Gateway, Subnet, DNS
	// For static IP uncomment WiFi.config below
	WiFi.mode(WIFI_STA);	
  //WiFi.config(foxNodeIp, IPAddress(UAS_Gateway), IPAddress(UAS_Subnet),IPAddress(UAS_DNS));

  putToDebugWithNewline("WiFiInit() Looking for SSID: "+String(WIFI_SSID),2);
  putToDebugWithNewline("WiFiInit()    with Password: "+String(WIFI_PASSWORD),2);
  putToDebugWithNewline("WiFiInit(): calling WiFi.begin", 3);
  
	//For Static IP uncomment WiFi.begin, comment if statement and edit whitespacing
  //WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	if (!connectWiFi_DHCP(WIFI_SSID, WIFI_PASSWORD)){
		delay(1000);

  	putToDebugWithNewline("- glob_connectedToDrone: "+String(glob_connectedToDrone),4);
  	putToDebugWithNewline("- wantToPaintDisplay: "+String(wantToPaintDisplay),4);
  	WiFi.setTxPower(WIFI_POWER_7dBm);                                // Set Tx power low (for Stage 2 this setting works well) NOTE: power def --> https://github.com/espressif/arduino-esp32/blob/70786dc5fa51601f525496d0f92a220c917b4ad9/libraries/WiFi/src/WiFiGeneric.h#L47
  	putToDebugWithNewline("WiFiStationConnected () - WiFi-STA Tx Power: "+String(WiFi.getTxPower()), 2); // Check power setting and send to Serial
  	putToDebugWithNewline("WiFiInit(): Exiting\n",2);
	}
}

void WiFiDropConnection(void){                                       // this subroutine is called when the state machine wants to drop the connection (if it exists) for a "back off" period 
	putToDebugWithNewline("WiFiDropConnection()", 3);
  if(httpConnectedToWiFi()){
		WiFi.disconnect(true);			                                     // TODO do we know if we get an error if you try to disconnect
		glob_connectedToDrone = 0;
		glob_droneConnectJustDropped = 0;
		delay(1000);
  }					
}

void setup_webserver_routing(void){
	webServer.onNotFound(handleNotFound);                                                     // handle 404
	webServer.begin();
	Serial.println("setup_webserver_routing() done");
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

unsigned int httpConnectedToWiFi(void){                              // TODO remove ? 
  return glob_connectedToDrone;

  if (WiFi.status() != WL_CONNECTED) {
		//glob_connectedToDrone = 0;				                             // this var will tell SM we lost connectivity.
		return 0;
	}
	return 1;
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
	tft_display(3);  // (BLACK) update display to signal HTTP Rx
}

// Mary had a an API
// with definitions neat
// but when the drone got sensor values
// it really was quite sweet

