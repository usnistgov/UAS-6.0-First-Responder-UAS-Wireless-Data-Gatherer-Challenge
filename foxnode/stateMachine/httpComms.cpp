// HTTP ESP32 specific interface routines
// =============================================================
// httpComms.cpp
// This file determines connection actions regarding http state and interacton
// with Wi-Fi connection status
// =============================================================
#include "httpComms.h"
#include "state.h"
#include "serial.h"
#include "timer.h"
#include "display.h"
#include "sensor.h"						          // For Wire (SPI)
#include "RingBuffer.h"					        // For Ringbuffer class "ringbuff"
#include "eeprom.h"
#include "WiFi.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "secrets.h"
#include <cstring>

// Forward declarations (configureMtlsClient uses these).
static void printTlsLastError(WiFiClientSecure &client);
static void dumpFirstBadBase64Char(const char *label, const String &s);

// Keep PEM buffers alive for the lifetime of the program.
// WiFiClientSecure commonly stores pointers to PEM buffers (does not always deep-copy),
// so local Strings can lead to dangling pointers after a helper returns.
static bool g_mtlsCacheLoaded = false;
static bool g_mtlsCacheOk = false;
static String g_caPem;
static String g_certPem;
static String g_keyPem;

static void configureMtlsClient(WiFiClientSecure &client) {
   // Load once into static storage so PEM pointers stay valid.
   // NOTE: WiFiClientSecure may store PEM pointers, not copies.
   // PEM buffers MUST outlive the TLS session.

  if (!g_mtlsCacheLoaded) {
    g_mtlsCacheLoaded = true;
    g_mtlsCacheOk = secretsGetMtls(g_caPem, g_certPem, g_keyPem);

    Serial.print("mTLS loaded from NVS: ");
    Serial.println(g_mtlsCacheOk ? "YES" : "NO");

    if (g_mtlsCacheOk) {
      // Optional sanity checks (safe: no secret content printed)
      dumpFirstBadBase64Char("CA", g_caPem);
      dumpFirstBadBase64Char("CRT", g_certPem);
      dumpFirstBadBase64Char("KEY", g_keyPem);
    }
  }

  if (g_mtlsCacheOk) {
    // IMPORTANT: Actually configure the secure client with the mTLS materials.
    client.setCACert(g_caPem.c_str());
    client.setCertificate(g_certPem.c_str());
    client.setPrivateKey(g_keyPem.c_str());
  } else {
    // Optional: compiled fallback for development only.
    // Keep this disabled in production.
    #ifdef FOXNODE_MTLS_FALLBACK_COMPILED
    client.setCACert(DRONE_SERVER_CA_CERT);
    client.setCertificate(FOXNODE_CLIENT_CERT);
    client.setPrivateKey(FOXNODE_CLIENT_KEY);
    #endif
  }

  if (!g_mtlsCacheOk) {
  Serial.println("FATAL: mTLS material missing");
}

  // Give the handshake more time during debug; 5s can be tight on some APs / servers.
  client.setHandshakeTimeout(15);
  client.setTimeout(15000);
}

// Print TLS stack error information for debugging mTLS / certificate verification issues.
static void printTlsLastError(WiFiClientSecure &client) {
  char errbuf[256];
  errbuf[0] = 0;
  client.lastError(errbuf, sizeof(errbuf));
  Serial.print("TLS lastError: ");
  Serial.println(errbuf[0] ? errbuf : "<none>");
}

// Print any bad PEM characters
static void dumpFirstBadBase64Char(const char *label, const String &s) {
  bool inBody = false;
  int lineStart = 0;

  while (lineStart < s.length()) {
    int lineEnd = s.indexOf('\n', lineStart);
    if (lineEnd < 0) lineEnd = s.length();
    String line = s.substring(lineStart, lineEnd);

    if (line.startsWith("-----BEGIN ")) {
      inBody = true;
      lineStart = lineEnd + 1;
      continue;
    }
    if (line.startsWith("-----END ")) {
      inBody = false;
      lineStart = lineEnd + 1;
      continue;
    }

    if (inBody) {
      for (int i = 0; i < line.length(); i++) {
        char c = line[i];
        bool ok =
          (c >= 'A' && c <= 'Z') ||
          (c >= 'a' && c <= 'z') ||
          (c >= '0' && c <= '9') ||
          c == '+' || c == '/' || c == '=';

        if (!ok) {
          Serial.print(label);
          Serial.print(" bad base64 char dec=");
          Serial.println((int)(uint8_t)c);
          return;
        }
      }
    }

    lineStart = lineEnd + 1;
  }

  Serial.print(label);
  Serial.println(" base64 looks clean");
}

static void configureSecureClientCommon(WiFiClientSecure &client) {
  client.setHandshakeTimeout(15);
  client.setTimeout(15000);
}

//The following block fist tries DHCP, than falls back to static IP if Drone server
// has AP, but no DHCP configured.
// Static fallback settings (edit to match your network)
static bool waitForIP(uint32_t timeoutMs) {
  uint32_t start = millis();
  while ((millis() - start) < timeoutMs) {
    if (WiFi.status() == WL_CONNECTED) {
      IPAddress ip = WiFi.localIP();
      if (ip[0] != 0) return true;  // not 0.0.0.0
    }
    delay(250);
  }
  return false;
}

bool connectWiFi_DHCP_thenStaticEveryAttempt(WifiProfile profile,
                                             IPAddress fallbackIp,
                                             uint32_t dhcpTimeoutMs,
                                             uint32_t staticTimeoutMs) {
  WiFi.persistent(false);

  String ssid, psk;
  if (!secretsGetWiFi(profile, ssid, psk)) {
    putToDebugWithNewline("WiFi secrets missing in NVS", 1);
    return false;
  }

  // Disable auto-reconnect for this connect attempt so we control sequencing.
  // (We can re-enable it after success if you prefer.)
  WiFi.setAutoReconnect(false);

  // -------- HARD reset WiFi stack --------
  WiFi.disconnect(true, true);   // wifioff=true, eraseap=true
  delay(200);
  WiFi.mode(WIFI_OFF);
  delay(200);
  WiFi.mode(WIFI_STA);
  delay(100);

  // -------- DHCP attempts --------
  const int dhcpAttempts = 4;         // try several times before static fallback
  const uint32_t betweenAttemptsMs = 1500;

  for (int attempt = 1; attempt <= dhcpAttempts; attempt++) {
    // Force DHCP mode (clears any prior static config)
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    delay(50);

    //WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    WiFi.begin(ssid.c_str(), psk.c_str());

    if (waitForIP(dhcpTimeoutMs)) {
      // Optional: allow WiFi library to handle future link drops
      WiFi.setAutoReconnect(true);
      return true;
    }

    // Failed DHCP attempt: fully stop and retry after a short wait.
    WiFi.disconnect(true, true);
    delay(betweenAttemptsMs);
    WiFi.mode(WIFI_OFF);
    delay(200);
    WiFi.mode(WIFI_STA);
    delay(100);
  }

  // -------- Static fallback --------
  WiFi.disconnect(true, true);
  delay(200);
  WiFi.mode(WIFI_STA);
  delay(50);

  WiFi.config(fallbackIp, UAS_Gateway, UAS_Subnet, UAS_DNS);
  //WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.begin(ssid.c_str(), psk.c_str());

  if (waitForIP(staticTimeoutMs)) {
    WiFi.setAutoReconnect(true);
    return true;
  }

  return false;
}

WebServer webServer(80);				        // Initialize Server w/ def HTTP port assignment
IPAddress netmask(255,255,0,0);			    // Entire UAS echosystem is a 16 bit classless nework
IPAddress clientIpAddr(192,168,40,30);	// Base IP for FoxNode IP range, IPs actually start at 80, as defined below (Don't change this)
IPAddress serverIpAddr(192,168,40,20);	// The fixed IP address of the Drone Server
IPAddress dnsServer(8,8,8,8);			      // If client wants to reach out over the internet

int httpResponseCode = -999;					// Standard HTTP responses like 200 and 400, -999 means not attemped.
// TLS status for display/debug
volatile uint8_t tlsState = TLS_STATE_NA;
char tlsLastError[48] = {0};

// Capture last TLS error into tlsLastError (short, display-safe)
static void tlsCaptureLastError(WiFiClientSecure &client) {
  char errbuf[256];
  errbuf[0] = 0;
  client.lastError(errbuf, sizeof(errbuf));
  strncpy(tlsLastError, errbuf[0] ? errbuf : "ERROR - Generic error", sizeof(tlsLastError) - 1);
  tlsLastError[sizeof(tlsLastError) - 1] = 0;
}

unsigned int httpWiFiState;
unsigned int wifiDropDeadTimer;			// Loss of signal to try to connect again or "dead time"
unsigned int WiFiHaveDhcpReply;			// True if have had a good DNS reply so the client has an IP address

void initializeHttpMsmValues(void) {	// Initialize the WiFi connect state monitor, used to create clean logging for WiFi conectivilty events
	httpWiFiState = 0;
	WiFiHaveDhcpReply = 0;				// Only called once on boot
}

// This routine is used to determine static IP settings
IPAddress getFoxNodeIP(unsigned short thisFoxNodeId){		// Formulate IP address based on Fox-Node ID
	if(thisFoxNodeId > 60)thisFoxNodeId = 0;				      // Logic for checking FoxNode ID, Don't assign over 175.
	return IPAddress(192, 168, 40, thisFoxNodeId + 80);   // Add 80 to FoxNode ID to keep it out of lower ranges for static devices
}

////////  next subroutine are tied to Wifi events
// IRQ like event where the sesnor client has connected to the Drone Server WiFi net.
// This would be just seeing and connecting to the Wifi network, possibly before DHCP negotiation

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
  wantToPaintDisplay = 1;			// (BLUE) display connection info
}

// Only mark connected once we actually have a non-zero IP address
void WiFiStationGotIP(WiFiEvent_t event, WiFiEventInfo_t info){
  IPAddress ip = WiFi.localIP();
  if (WiFi.status() == WL_CONNECTED && ip[0] != 0) {
    glob_connectedToDrone = 1;
  }
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
	glob_connectedToDrone = 0;                                       // STATE MACHINE FLAG: lock Sensor Dump to Server given no UAS // NOTE: that glob_connectedToDrone is an int, but used as a bool.
	glob_droneConnectJustDropped = 1;                                  // Update Global to show SMS that we are in a WiFi dissconnection state
	wantToPaintDisplay = 2;			                                       // (ORANGE) display disconnection message
  // print the reason so we know if it’s AUTH_FAIL, NO_AP_FOUND, etc.
  Serial.print("WiFi disconnected, reason=");
  Serial.println(info.wifi_sta_disconnected.reason);
}
//This subroutine attaches event handlers to subroutines for key WiFi evernts.

static uint32_t lastWifiAttemptMs = 0;
bool ensureWifiConnected(IPAddress fallbackIp) {
  // If already connected with a real IP, nothing to do
  if (WiFi.status() == WL_CONNECTED && WiFi.localIP()[0] != 0) {
    return true;
  }

  // Backoff so we don't thrash WiFi.begin() repeatedly
  static uint32_t lastAttemptMs = 0;
  if (millis() - lastAttemptMs < 5000) {
    return false;
  }
  lastAttemptMs = millis();

  // Try DHCP first, then static fallback
  //return connectWiFi_DHCP_thenStaticEveryAttempt(fallbackIp);
  return connectWiFi_DHCP_thenStaticEveryAttempt(WifiProfile::UAS6, fallbackIp);
}


void WiFiInit(IPAddress foxNodeIp){
	WiFi.setSleep(false);
  putToDebugWithNewline("WiFiInit() Starting WiFi", 2);
  // Load WiFi SSID and PSK from NVS
  secretsInit();
  // Attach handlers FIRST so we don't miss the initial connect event
	WiFi.onEvent(WiFiStationConnected,    WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
	WiFi.onEvent(WiFiStationGotIP,        WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
	WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);


  WiFi.mode(WIFI_STA);

  // Now connect (DHCP first, then static fallback)
 bool ok = ensureWifiConnected(foxNodeIp);

  // If we connected before the event handler fires (or event doesn't fire),
  // explicitly set the same flags your event handler would set.
  if (ok && WiFi.status() == WL_CONNECTED && WiFi.localIP()[0] != 0) {
    glob_connectedToDrone = 1;
    wantToPaintDisplay = 1;
  } else {
    glob_connectedToDrone = 0;
  }

  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  putToDebugWithNewline("WiFiInit(): Exiting\n", 2);
}


void WiFiDropConnection(void){                                       // this subroutine is called when the state machine wants to drop the connection (if it exists) for a "back off" period 
	putToDebugWithNewline("WiFiDropConnection()", 3);
  if(httpConnectedToWiFi()){
		WiFi.disconnect(true);			                                 
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

unsigned int httpConnectedToWiFi(void){
  if (WiFi.status() != WL_CONNECTED) return 0;
  IPAddress ip = WiFi.localIP();
  // Uncomment to show network config in serial monitor for debug
  //Serial.print("IP: ");   Serial.println(WiFi.localIP());
  //Serial.print("Mask: "); Serial.println(WiFi.subnetMask());
  //Serial.print("GW: ");   Serial.println(WiFi.gatewayIP());
  //Serial.print("DNS: ");  Serial.println(WiFi.dnsIP());

  if (ip[0] == 0) return 0;  // still 0.0.0.0 means no usable IP
  return 1;
}

void httpState_WiFiConnect(void){   // This is a state machine that deals with detecting/knowing if the foxnode client is connect to the drone sever.
  // This provides the functionality to try to get connected, and to be able to know when an "edge event"
  // has happpend, such as a transition from not connected to the drone WiFi to being connected.
    char s[64];  // Dynamically allocate memory char * to String conv....
    rssi_current_value = WiFi.RSSI();		// constantly update our receive RSSI
    switch(httpWiFiState){
      case 0:		// we are not connected, try to connect if requested
          // don't connect until there is a request to connect from the Main State Machine  (MSM)
          if(httpConnectedToWiFi() == 0)return;
          putToDebugWithNewline("HTTP SM 0: We are connected to WiFi",3);
          httpWiFiState = 1;
          return;
      case 1:		// If we are here, we are connected to WiFi
          if(glob_connectedToDrone == 0){     // Either the WiFi droped (from IRQ handler WiFiStationDisconnected() )
              putToDebugWithNewline("HTTP SM 1: Connection to Drone dropped",3);
              httpWiFiState = 3;
              wifiDropDeadTimer = WIFI_DROP_DEAD_WAIT;    // set in httpComms.h, typically 3 seconds
              return;
          }
          return;
      case 3:		// the MSM requested we drop the wifi connection, or the WiFi signal got too weak.
		// Now we go blind for WIFI_DROP_DEAD_WAIT, typically 3 seconds.
		// note that the MSM will impost a longer delay time after
		// requesting a WiFi disconnect of about 1 minute. Another timeout here is a "safety"
		// to keep things from getting crazy if the MSM is behaving badly
		if(wifiDropDeadTimer)return;
		putToDebugWithNewline("HTTP SM 3: Drone wait time over, resume monitoring WiFi state",2);
		httpWiFiState = 0;                                  // After the safety time has elapsed, we go back to waiting
		return;                 // to connect to the server.
    }
    // If we fall through, we have a bad state value
    sprintf(s,"* State %d is out of range- fell through the switch, resetting to zero", httpWiFiState);
    putToDebug(s, 1);
    httpWiFiState = 0;
}

void wifi_sendPost(const String& payload){		                     // Compatibility with MSM link independent view of the world
	sendHttpPost(payload);
}

void sendHttpPost(const String& payload) {  // The payload for the HTTP post has been created and can be sent to the Server
  if (!httpConnectedToWiFi()) {
    putToDebugWithNewline("sendHttpPost(): WiFi not connected! **POST Failed**", 1);
    return;
  } else {
		// Reset TLS status for this HTTPS attempt
		tlsState = TLS_STATE_NA;
		tlsLastError[0] = 0;
  }

  WiFiClientSecure secureClient;
  configureMtlsClient(secureClient);

  HTTPClient http;
  http.begin(secureClient, UAS_Server);
  http.addHeader("Content-Type", "application/json");

  // Avoid dumping full payload to serial (can stall)
  putToDebugWithNewline("SEND HTTP POST: payload bytes=" + String(payload.length()), 4);
  putToDebugWithNewline("About to call http.POST()", 2);
  httpResponseCode = http.POST(payload);
  putToDebugWithNewline("Returned from http.POST()", 2);
  putToDebug("HTTP Response code: ", 4);
  putToDebugWithNewline(String(httpResponseCode), 4);

  if (httpResponseCode == 200) {
    tlsState = TLS_STATE_OK;
		tlsLastError[0] = 0;
    String responseJSON = http.getString();
    putToDebug("sendHttpPost() Server POST responseJSON: ", 4);
    putToDebugWithNewline(String(responseJSON), 4);
    putToDebugWithNewline("", 4);

    deserializeJson(postRxPayload_doc, responseJSON);
    state_processSCRVReply();
  } else {
    tlsState = TLS_STATE_FAIL;
		tlsCaptureLastError(secureClient);
    putToDebug("Error: ", 1);
    putToDebugWithNewline(String(httpResponseCode), 1);

    String httpErr = http.errorToString(httpResponseCode);
    putToDebugWithNewline("HTTPClient error: " + httpErr, 1);

    putToDebugWithNewline("WiFi.status=" + String((int)WiFi.status()) +
                          " IP=" + WiFi.localIP().toString(), 1);
    putToDebugWithNewline("Target=" + String(UAS_Server), 1);
    putToDebugWithNewline("", 1);
  }
  http.end();
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
	tft_display(3);  // (Green) update display to signal HTTP Rx
}
