#include <Arduino.h>
#include <Wire.h>
#include "HDC1080Wrapper.h"

// Create an instance of the wrapper class
GuL::HDC1080Wrapper hdcWrapper(Wire);

String outputFormat = "Temperature \t= % 6f °C \n"
                      "Humidity \t= % 6f %% \n"
                      "\n";

void setup() {
    Serial.begin(9600);
    Wire.begin();

    // Initialize and configure the sensor
    hdcWrapper.begin();
    hdcWrapper.configure();
}

void loop() {
    // Start acquisition and wait for conversion
    hdcWrapper.startAcquisition();

    // Read temperature and humidity
    float temperature = hdcWrapper.readTemperature();
    float humidity = hdcWrapper.readHumidity();

    // Print the results to serial monitor
    Serial.printf(outputFormat.c_str(), temperature, humidity);
}
