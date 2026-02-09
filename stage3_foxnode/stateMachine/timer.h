#ifndef TIMER_H
#define TIMER_H

// SCOPE: Setup and opperate all timing functions in this system: I2C-RTC, ESP timer ISR, NTP setup

// Includes
#include "display.h"
#include "sensor.h"

// Global Variables
extern time_t myUnixTime;                             // FN base time from RTC
extern struct tm myTime;                              // must implement a "tm struct" to get/set RTC time. However Unix time is for all transmisions [seconds]
extern time_t ftsr_time;                              // For use in General POST newest ringbuffer entrie 
extern hw_timer_t *timer1;                            // ISR for Sensor data state machine  
extern hw_timer_t *timer2;                            // ISR for General POST (whoami)
extern bool Timer1_flag;                              // Timer flag to indicate interrupt
extern bool Timer2_flag;                              // Timer flag to indicate interrupt
extern unsigned int sensorSampleRateTimer;            // How often to stuff data into the Ringbuffer

// Function Declaration
void initTimers(void);                                // Setup ESP32 timer ISR timer1 & timer2 
int initNTP(void);                                    // Look for hotspot during system setup, if found set NTP. if not found read RTC time val an carry on 
time_t rtcToUnixTimeStamp(struct tm rawTime);         // RTC func to handle conversion from 
struct tm unixTimeStampTortc(time_t myUnixTime);      // RTC func to handle conversion from 

#endif    //  TIMER_H
