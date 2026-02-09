//
// Timer code. timer 1 is a 10 mS timer that can take a number of globals
// and "decrement and stick at zero" them every clock tick. Many of the
// state machine timers are defined in stateGlobal.h, and start with "glob_xxxxx"
// Timer 2 is a microsecond timer that is just a free-running timer with
// no IRQs. Software can grab the 32-bit timer in order to make time
// measurements.

#include <Arduino.h>		
#include <time.h>				                                                            // Print functions 
#include "timer.h"
#include "sensor.h"							// for sensorSampleRate
#include "state.h"
#include "secrets.h"
#include <WiFi.h>
#include <sys/time.h>
#include "secrets.h"
#include "httpComms.h" 

unsigned int sensorSampleRateTimer;			// timer that decremants to zero to control when to take a sample from the sensors

hw_timer_t *timer1 = NULL;					// ISR for Sensor data state machine
hw_timer_t *timer2 = NULL;					// ISR for General POST (whoami)

time_t myUnixTime = time(nullptr);
bool  Timer1_flag = false;					// ISR Flag set on each IRQ (currently every 10 mS). Decraments various counters used by main program.
bool  Timer2_flag = false;					// ISR Flag for timer 2, which fires once and then never again.

void IRAM_ATTR onTimer1ISR(){				// Timer 1 IRQ Interrupt Service Routine (ISR) 100 Hz / 10 mS
	if(wifiDropDeadTimer)wifiDropDeadTimer--;
	if(glob_timer_droneReplyWait)glob_timer_droneReplyWait--;
	if(glob_timer_poll_delay)glob_timer_poll_delay--;
	if(wifiDroneBlindTimer)wifiDroneBlindTimer--;		// blind time timer for after a good transction with the Drone Server
	if(sensorSampleRateTimer)sensorSampleRateTimer--;
	Timer1_flag = true;						                                                            // Set flag to indicate we need to do something in the main loop, controlss the 10 [mS] loop time
}

void IRAM_ATTR onTimer2ISR() {					// Timer 2 Interrupt Service Routine (ISR) 10 KHz / 100 uS
    // this is ignored, only fires once.
}

void initTimers(void){										// setup ESP32 timers ISR (these drive StateMachine opp.)
  Timer1_flag = false;										// ISR interupt flag for Timer1
  Timer2_flag = false;										// ISR interupt flag for Timer2

  sensorSampleRate = SENSOR_SAMPLE_RATE;					// move default sameple rate into a variable. FUTURE get this from the EEPROM in the RTC
  sensorSampleRateTimer = 0;								// Sample sensors timer. Set to zero (expired) so we get sample immeidatly after boot.
  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	// Note that this means that EEPROM updates must happen after the "initxxxx" subroutines.
  timer1 = timerBegin(1000);				 				// Set up timer with 1 KHz / 1 mS frequency
  timerAttachInterrupt(timer1, onTimer1ISR);					// Attach the onTimer1ISR interrupt function to the timer
  timerAlarm(timer1, 10, true, 0);							// Repeat ISR every 10 mS

  timer2 = timerBegin(10000);								// Set up timer with 10 kHz / 100 uS frequency
  timerAttachInterrupt(timer2, onTimer2ISR);				// Attach the onTimer1ISR interrupt function to the timer
  timerAlarm(timer2, 10, false, 0);							// Timer IRQ (which is not used) is every 1 mS
} 

int initNTP(void){
  unsigned long startMs;
  unsigned long timeoutMs;

  // Screen 5 = primary NTP setup (UAS_NTP)
  tft_display(5);
  putToSerialWithNewline("**FN-NTP RV-3032 Setup Initiated**");

  // Init RTC (RV-3032)
  secretsInit();
  int RTC_err = rtc.init();
  if (RTC_err != RTC_SUCCESS) {
    putToSerialWithNewline("{ERROR} RTC Failure, stopping... does this FN have a SEN's PCB board connected (I2C)?");
    return -1;
  }

  // Seed from RTC first
  rtc.getTime(&myTime);

  // Helper: commit current myTime to RTC + TFT
  auto commitTimeToRtc = [&](){
    rtc.setTime(&myTime);
    RTC_tftPrint();
    putToSerial("FN-NTP Time Set:" + String(myTime.tm_hour) + String(myTime.tm_min) + String(myTime.tm_sec));
  };

  // Helper: SNTP sync from a server (hostname or IP). Accept whatever time it provides.
  auto sntpSync = [&](const char *server, uint32_t waitMs) -> bool {
    configTime(0, 0, server);
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, waitMs)) {
      return false;
    }
    myTime = timeinfo;
    commitTimeToRtc();
    return true;
  };

  // ----------------------------
  // Primary: connect to UAS_NTP and sync via Internet NTP
  // ----------------------------
  String ntpSsid, ntpPsk;
  bool haveNtpWiFi = secretsGetWiFi(WifiProfile::NTP, ntpSsid, ntpPsk);

  if (haveNtpWiFi && ntpSsid.length() > 0) {
    putToSerialWithNewline("Attempting NTP WiFi Connection to SSID " + ntpSsid);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    delay(100);
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE); // DHCP

    startMs = millis();
    timeoutMs = 30000;
    WiFi.begin(ntpSsid.c_str(), ntpPsk.c_str());

    while (WiFi.status() != WL_CONNECTED) {
      if (millis() - startMs >= timeoutMs) {
        putToSerialWithNewline("UAS_NTP timeout, switching to Drone Server NTP fallback");
        WiFi.disconnect(true);
        delay(200);
        break;
      }
      delay(500);
      putToSerial(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      putToSerialWithNewline("NTP WiFi Connected");
      tft.println("NTP WiFi Connected");

      if (sntpSync("pool.ntp.org", 8000)) {
        putToSerialWithNewline("Internet NTP sync OK");
        WiFi.disconnect(true);
        delay(500);
        tft.println("NTP WiFi Disconnect");
        return 0;
      }

      putToSerialWithNewline("Internet NTP sync failed, switching to Drone Server NTP fallback");
      WiFi.disconnect(true);
      delay(200);
    }
  } else {
    putToSerialWithNewline("NTP WiFi secrets missing in NVS, switching to Drone Server NTP fallback");
  }

  // ----------------------------
  // Fallback: connect to uas6 and query Drone Server as NTP server
  // ----------------------------
  tft_display(6);
  putToSerialWithNewline("Attempting Drone Server NTP fallback (uas6)...");

  String uasSsid, uasPsk;
  if (!secretsGetWiFi(WifiProfile::UAS6, uasSsid, uasPsk) || uasSsid.length() == 0) {
    putToSerialWithNewline("UAS6 WiFi secrets missing in NVS, cannot do Drone Server NTP fallback");
    return 0;
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE); // DHCP

  startMs = millis();
  timeoutMs = 20000;
  WiFi.begin(uasSsid.c_str(), uasPsk.c_str());

  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startMs >= timeoutMs) {
      putToSerialWithNewline("Drone Server NTP fallback: WiFi connect timeout");
      WiFi.disconnect(true);
      delay(200);
      return 0;
    }
    delay(500);
    putToSerial(".");
  }

  putToSerialWithNewline("uas6 WiFi Connected (fallback)");
  tft.println("uas6 WiFi Connected");
  tft.println("Querying Drone NTP");

  if (sntpSync("192.168.40.20", 8000)) {
    putToSerialWithNewline("Drone Server NTP fallback OK");
  } else {
    putToSerialWithNewline("Drone Server NTP fallback failed, proceeding with RTC time");
  }

  WiFi.disconnect(true);
  delay(500);
  return 0;
}

time_t rtcToUnixTimeStamp(struct tm rawTime){			// Convert RTC time (dat and HH:MM:SS into Unix time (seconds from epoch)
	myUnixTime = mktime(&myTime);
	return myUnixTime;
}

struct tm unixTimeStampTortc(time_t myUnixTime){ 		// Convert unix time (seconds from epoch) into RTC time format (ISO 8601-ish)                                           //
	myTime = *localtime(&myUnixTime);
	return myTime;
}