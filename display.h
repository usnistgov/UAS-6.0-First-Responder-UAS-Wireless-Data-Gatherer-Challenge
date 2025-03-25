#ifndef DISPLAY_H
#define DISPLAY_H

// SCOPE: globals for code that writes things to the onboard ESP display (TFT)

// Includes
#include <Arduino.h>
#include <Adafruit_GFX.h>                                        // Display hardware def.
#include <Adafruit_ST7789.h>                                     // Hardware-specific library for ST7789
#include "serial.h"
#include "sensor.h"                                              // JSON lib
#include "state.h"                                               // for UAS_Server var

// Global Variables
extern Adafruit_ST7789 tft;
extern short disp_rotation;
extern unsigned int wantToPaintDisplay;

// Function Declarations
extern void RTC_tftPrint();			
extern void initializeDisplay(void);
extern void tft_display(int disp_update);

void RTC_tftPrint();

#endif  // DISPLAY_H
