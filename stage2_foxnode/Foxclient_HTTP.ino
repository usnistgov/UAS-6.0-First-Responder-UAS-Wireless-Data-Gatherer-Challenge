/*
 * NIST-PSCR psprizes@nist.gov - UAS 6.0 First Responder UAS Data Gatherer Challenge Stage 2 - "Fox-Node" HTTP IoT Client  
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/index.html
 * https://github.com/espressif/arduino-esp32
 * https://datatracker.ietf.org/doc/html/rfc2616 
 * https://arduinojson.org/v7/how-to/use-arduinojson-with-httpclient/ 
 */

#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

const char* serverName = "http://192.168.1.1/update";               // URI for POST Server  
const int httpPort = 80;                                            // def port assignment 
const char* ssid = "UAS6";                                      // UAS 6.0 Test Network SSID
const char* password = "UAS6";                                  // UAS 6.0 Test Network PSW
char* http_payload = "NULL";                                        // Var for HTTP payload storage
long pkt_count = 0;                                                 // Client packet counter
int httpResponseCode = -1;                                          // Client resp codes
String httpDummyData = "Null";                                      // Client Payload var

                                                                    // Includes for display
#include <Adafruit_GFX.h>                                           // Display hardware def.
#include <Adafruit_ST7789.h>                                        // Hardware-specific library for ST7789
#include <SPI.h>

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);     // Use dedicated hardware SPI pins for TFT disp

#include "ArduinoJson.h"
DynamicJsonDocument doc(2048);                                      // Create an empty document

unsigned long lastTime = 0;             // easy mode time var setup
unsigned long timerDelay = 10000;        // Timer set to 10 seconds (10000) Update this var for POST msg loop timing

void tft_display(int disp_update){ 
/*disp key 
 3 - on sent POST (GREEN)
 2 - no connection to HTTP server (Orange)
 1 - On WiFi got IP 
 0 - TFT init code (only call during setup)
*/
 short disp_rotation = 1;                     // update to rotate TFT display: input "1" through "4"                   
  if (disp_update ==3){
    tft.fillScreen(ST77XX_GREEN);tft.setCursor(0, 0);
    tft.setTextSize(3);tft.setTextColor(ST77XX_WHITE);tft.println("FOX > UAS 6.0");
    tft.setTextSize(2);tft.setTextColor(ST77XX_RED);tft.setTextSize(2);tft.print("IP: ");tft.println(WiFi.localIP());
    tft.print("WiFi RSSI: ");tft.println(WiFi.RSSI()); // THIS CALL SEEMS NOT TO WORK 
    tft.print("Sent HTTP POST# ");tft.println(pkt_count);
    tft.println("Payload: ");tft.println(httpDummyData);
    tft.println("HTTP Response code: ");tft.println(httpResponseCode);
    tft.setTextColor(ST77XX_WHITE);             // Set back to white for later calls
  }else if(disp_update == 2){                   // no HTTP Server connection aval
    tft.fillScreen(ST77XX_ORANGE);tft.setCursor(0, 0);
    tft.setTextSize(3);tft.setTextColor(ST77XX_WHITE);tft.println(("FOX > UAS 6.0"));         
    tft.setTextSize(2);tft.println("HTTP Server Connection Failed.");
    tft.println("");tft.println("Reconnteting...");
    tft.println("Host URI:  ");tft.println(serverName);
  }else if(disp_update == 1){                  // (BLUE) disp IP connected
    tft.fillScreen(ST77XX_BLUE);tft.setCursor(0, 0);
    tft.setTextSize(3);tft.setTextColor(ST77XX_WHITE);tft.println("FOX> UAS 6.0");
    tft.setTextSize(2);tft.println("WiFi Connected.");
    tft.print("IP: ");tft.println(WiFi.localIP());
  }else if(disp_update == 0){                  // (GREEN) disp initial setup  screen
    pinMode(TFT_BACKLITE, OUTPUT);             // Start TFT &turn on backlite
    digitalWrite(TFT_BACKLITE, HIGH);
    tft.init(135, 240);                         // ST7789 (TFT display) 240x135
    tft.setRotation(disp_rotation);tft.setCursor(0, 0);tft.fillScreen(ST77XX_GREEN);
    tft.setTextSize(3);tft.setTextColor(ST77XX_WHITE);tft.println("FOX> UAS 6.0");tft.println("INITIALIZED");
    tft.setTextSize(2);                         // End TFT init
  }else{                                        //TODO err message/handeling here? 
    Serial.println("Error in display function...");  
    Serial.println("tft_display(");Serial.println(disp_update,DEC);Serial.print(")");
  }
}

// start of WiFi Callbacks (for network reconnection)
void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){  
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("WiFi connected"); 
  WiFi.setTxPower(WIFI_POWER_7dBm);              // Set Tx power low (for Stage 2 this setting works well) NOTE: power def --> https://github.com/espressif/arduino-esp32/blob/70786dc5fa51601f525496d0f92a220c917b4ad9/libraries/WiFi/src/WiFiGeneric.h#L47
  Serial.print("WiFi Tx Power: ");Serial.println(WiFi.getTxPower());  // Check pwr setting and send to Serial
  Serial.println("IP address: ");Serial.println(WiFi.localIP());
  tft_display(1);                                // disp connection info
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.print("WiFi Station: ");Serial.print(event);Serial.print(" Reason: ");Serial.println(info.wifi_sta_disconnected.reason);
  Serial.println("Trying to Reconnect");
  WiFi.disconnect();                             
  WiFi.reconnect();                              
  tft_display(2);                                // disp disconnection message
}
// End of WiFi Callbacks 

void setup() { 
  Serial.begin(115200);        // Start Serial interface
  WiFi.disconnect(true);       // init WiFi
  tft_display(0);              // init TFT for ESP32 dispaly
  delay(1000);

  WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);              // Implement WiFi Callback functions to avoid handles in main loop 
  WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);                            
  WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);        // Handle WiFi reconnect 
  WiFi.begin(ssid, password);
  Serial.println("**Fox-Node Initialized**");
}

void loop() 
{
  if ((millis() - lastTime) > timerDelay) {       // Try to send a HTTP-POST every "timerDelay" [10 s]
    if(WiFi.status()== WL_CONNECTED){             //Check WiFi connection status
      Serial.println("Attempting POST.");
      HTTPClient http;
      http.begin(serverName);Serial.println("Server connected, Sending POST.");
      http.useHTTP10(true);                                             // Dont use chunking, shouldnt given our small payload but best to define... 
      http.addHeader("Content-Type", "application/json");               // Specify content-type header JSON
      doc["Node_RSSI"]=WiFi.RSSI();                                     // Read WiFi Connection info into JSON doc
      serializeJson(doc,httpDummyData);                                 // create JSON HTTP-POST payload 
      httpResponseCode = http.POST(httpDummyData);                      // Send HTTP-POST request
      pkt_count = pkt_count + 1;                                        // Add to total packet count 
      if (httpResponseCode == 200){Serial.println("POST success.");
      }else if(httpResponseCode == HTTPC_ERROR_CONNECTION_REFUSED){     // NOTE: HTTPC_ERROR_CONNECTION_REFUSED == -1 https://github.com/espressif/arduino-esp32/blob/master/libraries/HTTPClient/src/HTTPClient.h
        WiFi.reconnect();           // Debug #1 client gets stuck on IP network but fails HTTP POST... recover from this state w/ reconnect
      }      

      Serial.print("STA PWR: ");Serial.println(WiFi.getTxPower());      // Check WiFi Tx power / send to serial
      Serial.print("HTTP Response code: ");Serial.println(httpResponseCode);
      Serial.print("POST Payload: ");serializeJson(doc,Serial);         // Display responce info to serial
      http.end();                                                       // Free HTTP resources
      tft_display(3);                                                   // Disp HTTP session POST responce info   
    }
    else {
      Serial.println("WiFi Disconnected");  // Network unavailable :ERR state WiFi will auto reconnect 
      if ((WiFi.status() != WL_CONNECTED) && (millis() - lastTime) > timerDelay) { // If timer is up and no server, reconnect
        WiFi.reconnect();                                               // TODO: call WiFi disconnect ISR manually
        tft_display(2);                                                 // disp no connection screen 
      } 
    }
    lastTime = millis();                    // Reset our counter
  }
}


