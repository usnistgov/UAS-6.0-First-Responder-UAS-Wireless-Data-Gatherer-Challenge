#import "display.h"

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);                          // Use dedicated hardware SPI pins for TFT disp

short disp_rotation = 3;			                                                           // Set display initial rotation TFT display: input "1" through "4"

// NOTE The RTC is on the sensor board, but this f() deals with printing.
void RTC_tftPrint() {                                                                    // TFT-display time as a formatted string (ISO 8601)
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  char formattedTime[20];
  snprintf(formattedTime, sizeof(formattedTime), "%04d-%02d-%02d %02d:%02d:%02d",
           myTime.tm_year + 1900, myTime.tm_mon + 1, myTime.tm_mday,
           myTime.tm_hour, myTime.tm_min, myTime.tm_sec);
  tft.println(formattedTime);
}

void tft_display(int disp_update) {
  //disp key 
  //5 - NTP Setup/Info               (YELLOW) 
  //4 - on rx HTTP update param"     (BLACK)
  //3 - on sent POST                 (GREEN)
  //2 - no connection to HTTP server (ORANGE)
  //1 - On WiFi got IP               (BLUE) 
  //0 - TFT init code                (GREEN)
  wantToPaintDisplay = 0;			// clear any pending request to paint the display
  if (disp_update == 5) {           // (YELLOW) disp FN NTP time setting 
    tft.setRotation(disp_rotation);tft.fillScreen(ST77XX_YELLOW);tft.setCursor(0, 0);
    tft.setTextSize(3);tft.setTextColor(ST77XX_WHITE);tft.println("FOX > UAS 6.0");
    tft.setTextSize(2);tft.println("NTP looking 4");
    tft.print("SSID: ");tft.println(NTP_WIFI_SSID);						// Display NTP Hotspot connection cred to disp.
    tft.print("PASS: ");tft.println(NTP_WIFI_PASSWORD);					// Display NTP Hotspot connection cred to disp.
  } else if (disp_update == 4) {    // (BLACK) display is built in, ST7789 (TFT display) 240x135
    tft.setRotation(disp_rotation);pinMode(TFT_BACKLITE, OUTPUT);digitalWrite(TFT_BACKLITE, HIGH);tft.init(135, 240);
    tft.setCursor(0, 0);tft.fillScreen(ST77XX_BLACK);tft.setTextSize(3);tft.setTextColor(ST77XX_WHITE); // End TFT init
    tft.println("FOX > UAS 6.0");tft.println("FN-API Rx");tft.setTextSize(2);                      
  }else if (disp_update == 3) {     // (GREEN)
    tft.setRotation(disp_rotation);tft.fillScreen(ST77XX_GREEN);
    tft.setCursor(0, 0);tft.setTextSize(3);tft.setTextColor(ST77XX_WHITE);tft.println("FOX > UAS 6.0");
    RTC_tftPrint();  // Display formatted RTC time info
    tft.setTextSize(2);tft.setTextColor(ST77XX_RED);
    tft.print("RSSI: ");tft.println(String(data_doc["rssi"]));			// Build our data display.... This is what has been pushed to the Ringbuffer and will be POSTED if UAS ask's
    tft.print("STA_TX_PWR: ");tft.println(String(data_doc["tpwr"]));
    tft.print("L: ");tft.println(String(data_doc["l"]));
    tft.print("H: ");tft.println(String(data_doc["h"]));
    tft.print("T: ");tft.println(String(data_doc["t"]));
    tft.print("P: ");tft.println(String(data_doc["p"]));
    tft.print("lat: ");tft.println(String(data_doc["lat"]));  
    tft.print("lon: ");tft.println(String(data_doc["lon"]));
    tft.print("elev: ");tft.println(String(data_doc["elev"]));
    tft.print("HTTP Response code: ");
    tft.println(httpResponseCode);
    tft.setTextColor(ST77XX_WHITE);										// Set back to white for later calls
  } else if (disp_update == 2) {      // (ORANGE) no WiFi/HTTP Server connection available 
    tft.setRotation(disp_rotation);tft.fillScreen(ST77XX_ORANGE);tft.setCursor(0, 0);tft.setTextSize(3);tft.setTextColor(ST77XX_WHITE);
    tft.println(("FOX > UAS 6.0"));tft.setTextSize(2);
    tft.println("Looking for Network");
    tft.print("SSID: ");tft.println(WIFI_SSID);							// Display NTP HotSpot connection cred to disp.
    tft.print("PASS: ");tft.println(WIFI_PASSWORD);						// Display NTP HotSpot connection cred to disp.
    RTC_tftPrint();
    tft.println("Reconnteting URI:  ");
    tft.println(UAS_Server);
  } else if (disp_update == 1) {     // (BLUE) disp IP connected
    tft.setRotation(disp_rotation);tft.fillScreen(ST77XX_BLUE);tft.setCursor(0, 0);tft.setTextSize(3);tft.setTextColor(ST77XX_WHITE);
    tft.println("FOX > UAS 6.0");tft.setTextSize(2);
    tft.println("WiFi Connected.");
    tft.print("IP: ");tft.println(WiFi.localIP());
    tft.print("RSSI: ");tft.println(String(WiFi.RSSI()));
    RTC_tftPrint();														// Display formatted RTC time info
  } else if (disp_update == 0) {    // (GREEN) disp initial setup  screen
    tft.init(135, 240);pinMode(TFT_BACKLITE, OUTPUT);digitalWrite(TFT_BACKLITE, HIGH);    // Start TFT &turn on backlite
    tft.setRotation(disp_rotation);tft.setCursor(0, 0);tft.fillScreen(ST77XX_GREEN);tft.setTextSize(3);
    tft.setTextColor(ST77XX_WHITE);tft.println("FOX > UAS 6.0");
    tft.println("INITIALIZED");tft.setTextSize(2);						// End TFT init
  } else {                                                                              
    Serial.println("Error in display function...");
    Serial.println("tft_display(");
    Serial.println(disp_update, DEC);
    Serial.print(")");
  }
}
