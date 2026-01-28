//
// Timer code. timer 1 is a 10 mS timer that can take a number of globals
// and "decrement and stick at zero" them every clock tick. Many of the
// state machine timers are defined in stateGlobal.h, and start with "glob_xxxxx"
// Timer 2 is a microsecond timer that is just a free-running timer with
// no IRQs. Software can grab the 32-bit timer in order to make time
// measurements.

#include <Arduino.h>						                                                            // Print functions 
#include "timer.h"
#include "sensor.h"							// for sensorSampleRate
#include "state.h"

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

// Initialize the RTC (Real Time Clock) chip. We connect to a hotspot with
//  Internet connectivity so that we can get the time via HTP protocol.
int initNTP(void){
	char s[128];
  unsigned long timeout;
	unsigned long startMillis;
  unsigned long postStartTimeMs;
  unsigned long pdt;						// how long it took to do the entire POST process 'p'ost 'd'elta 't'ime
	tft_display(5);											// (YELLOW) tftDisp NTP time setup
	putToSerialWithNewline("**FN-NTP RV-3032 Setup Initiated**");
	int RTC_err = rtc.init();								// init Real Time Clock (RV-3032 on SNS-PCB via I2C)
	if(RTC_err == RTC_SUCCESS){
		putToSerialWithNewline("RTC init Success");
		putToSerialWithNewline("RTC type = " + String(szRTCType[rtc.getType()]));
		rtc.getTime(&myTime);                                                                   // Set ESP Time from RTC
		putToSerialWithNewline("Set ESP time to RTC mem:");
		putToSerial(String(myTime.tm_year + 1900) + "-" + String(myTime.tm_mon + 1) + "-" + String(myTime.tm_mday) + " " + String(myTime.tm_hour) + ":" + String(myTime.tm_min) + ":" + String(myTime.tm_sec) + "\n");
	} else {
		putToSerialWithNewline("{ERROR} RTC Failure, stopping... does this FN have a SEN's PCB board connected (I2C)?");
		return -1;											// no clock found /  RTC error !
	}

	// START NTP setup This gets time by connecting go a local WiFi that can get to the Internet
	putToSerialWithNewline("Attempting NTP-WiFi Connection to SSID "+String(NTP_WIFI_SSID));
	putToSerialWithNewline("password is"+String(NTP_WIFI_PASSWORD));
	WiFi.mode(WIFI_STA);									// Otherwise opp seems to be WIFI_STA_AP mode... this makes the server connection very poor
	startMillis = millis();     // debug timing WiFi connect
	WiFi.disconnect(true);
	delay(100);
	WiFi.mode(WIFI_STA);
	WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);  // force DHCP, clear static
	WiFi.begin(NTP_WIFI_SSID, NTP_WIFI_PASSWORD);

	startMillis = millis();
	timeout = 30000;										// 30 seconds to get connected to net for NTP
	while( WiFi.status() != WL_CONNECTED ){
		if(millis() - startMillis >= timeout){
			putToSerialWithNewline("\nTimeout reached, proceeding with regular code flow. NTP unable to set");
			WiFi.disconnect(true);							// clean up WiFi STA from NTP attempt
			delay(1000);									// Short 1 second delay to ensure disconnect is complete. delay function units are mS
			return 0;										// sucsses RTC initalized
		}
		delay(500);
		putToSerial( "." );
	}
  pdt = millis() - postStartTimeMs;
  sprintf(s,"NTP Setup procTime=%d", pdt);
  putToDebugWithNewline(s,2);
	if(WiFi.status() == WL_CONNECTED){				// If we found NTP_WiFi read NTP and save to RTC
		putToSerialWithNewline("\nNTP WiFi Connected");
		tft.println("NTP WiFi Connected");
		timeClient.setTimeOffset(0);						// GMT - 7 = -25200    (MST selection) TODO: UPDATE for day of timezone??
		timeClient.begin();
		timeClient.update();
		time_t rawTime = timeClient.getEpochTime();			// get NTP time in epoch format (number of seconds from 1900)
		localtime_r(&rawTime, &myTime);						// convert local time and store in myTime struct
		rtc.setTime(&myTime);								      // set RCT time
    RTC_tftPrint();  // Display formatted RTC time info
		putToSerial("FN-NTP Time Set:"+String(myTime.tm_hour)+String(myTime.tm_min)+String(myTime.tm_sec));
		timeClient.end();									      // clean up NTP
		WiFi.disconnect(true);							  	// clean up WiFi STA
		delay(1000);										        // Short 1000 ms (1 second) delay to ensure disconnect is complete
		tft.println("NTP WiFi Disconnect");
		return 0;											          // sucsses RTC initalized
  }
  else{
    WiFi.disconnect(true);
    pdt = millis() - postStartTimeMs;
    sprintf(s,"NTP Setup failed, procTime=%d",pdt);
    putToDebugWithNewline(s,2);
    delay(1000);
  }
}

time_t rtcToUnixTimeStamp(struct tm rawTime){			// Convert RTC time (dat and HH:MM:SS into Unix time (seconds from epoch)
	myUnixTime = mktime(&myTime);
	return myUnixTime;
}

struct tm unixTimeStampTortc(time_t myUnixTime){ 		// Convert unix time (seconds from epoch) into RTC time format (ISO 8601-ish)                                           //
	myTime = *localtime(&myUnixTime);
	return myTime;
}