# GuL TI Humidity HDC10XX
**GuL TI Humidity HDC10XX** is a library for the Arduino-Framework to work with the humditiy sensors series HDC10XX (HDC1010 and HDC1080) from Texas Instruments



## Installation

1. [Arduino Library Manager (Recommended)](https://www.arduino.cc/en/Guide/Libraries)  
2. [Download or clone this repository into your arduino libraries directory](https://help.github.com/articles/cloning-a-repository/)  


## Usage
1. Include module  
   
   ```cpp
   #include <HDC1010.h> // Or HDC1080.h
   ```
   
2. Create a instance with the serial port it uses
```cpp
  //The modul uses the namespace GuL, instead of Serial1 you can use Serial, Serial2, or own instantiated HardwareSerial or SoftwareSerial
   GuL::HDC1010 hdc(Wire); 
```
   
3. In `setup()`, basic setup of the modul.  
   
   ```cpp
   void setup() {
      Wire.begin();
     hdc.resetConfiguration();
     hdc.enableHeater();
     hdc.setHumidityResolution(GuL::HDC1080::HumidityMeasurementResolution::HUM_RES_14BIT);
     hdc.setTemperaturResolution(GuL::HDC1080::TemperatureMeasurementResolution::TEMP_RES_14BIT);
     hdc.setAcquisitionMode(GuL::HDC1080::AcquisitionModes::BOTH_CHANNEL);
   }
   ```
   
4. In `loop()` of the sketch, run the object's **loop()** method.  
   
   ```cpp
   void loop() {
     hdc.startAcquisition(GuL::HDC1080::Channel::BOTH);
     delay(hdc.getConversionTime(GuL::HDC1080::Channel::BOTH)/1000);

     float temperature = hdc.getTemperature();
     float humidity = hdc.getHumidity();

     Serial.printf("%f °C \n %f % \n",temperature,humidity);
   }
   ```   



## APIs
### Constructors

   ```cpp   
      GuL_TI_Humidity_HDC10XX(TwoWire &wire); // Not recommended
      HDC1010(TwoWire &wire, uint8_t addr = 0b1000000);
      HDC1080(TwoWire &wire);
   ```

   The first constructor is not recommended, but results in the same behaviour like HDC1080(...).
   HDC1010 and HDC1080 are for the specific devices whereby the HDC1080 construction leads to the usage of the address 0b1000000 and with the the HDC1010(...) the address can be specified. The address have to be in the range of 0b1000000 up to 0b1000011, otherwise the default (0b1000000) will be used.

### methods

  ```cpp
        std::string getSensorName() { return _name; }
  ```
  Return the device name (Like HDC1010 or HDC1080)

  ```cpp
        bool startAcquisition(Channel chan);
  ```
  Start the acquisition of the given channel
  
  ```cpp
        bool setAcquisitionMode(AcquisitionModes mode);
  ```
  set the mode of the acquisition
  
  ```cpp
        bool resetConfiguration();
  ```
  Set the device configuration to default
  
  ```cpp
        bool enableHeater();
  ```
  Enable the heater, (further details)[#heater-details] 
  
  ```cpp
        bool disableHeater();
  ```
  Disable the heater (further details)[#heater-details]
  
  ```cpp
        bool heaterEnabled();
  ```
  Checks if the heater in enabled or disabled
  
  ```cpp
        bool setHumidityResolution(HumidityMeasurementResolution res);
  ```
  Set the conversion resolution of the humidity channel
  
  ```cpp
        bool setTemperaturResolution(TemperatureMeasurementResolution res);
  ```
  Set the conversion resolution of the temperature channel
  
  ```cpp
        BatteryStatus getBatteryStatus();
  ```
  Get the status of the battery, thereby it is assumpt that the battery is directly connected to the powerline of the sensor. If the voltage of the sensor if > 2.8V than the return is BatteryStatus::BATTERY_OK otherwise BatteryStatus::BATTERY_LOW
  
  ```cpp
        uint16_t getDeviceID();
  ```
  Get the device id (0x1000 for HDC1010 and 0x1050 for HDC1080)
  
  ```cpp
        uint16_t getManufacturerID();
  ```
  Get the manufacturer id (0x5449)
  
  ```cpp
        uint64_t getSerialID();
  ```
  Get the sensor unique id (40bit width)
  
  ```cpp
        uint16_t getConversionTime(Channel chan); // Returns the conversion time in µs!
  ```
  Get the needed time of conversion dependen of the resolution. The time is in µs!
  
  ```cpp

        float getTemperature(); // in °C
  ```
  Get the measured temperature in °C
  
  ```cpp
        float getHumidity();    // in %
  ```
  Get the relative humidity in %
  

  ### HDC1010 specific

  ```cpp
    void setDRDYnPin(uint8_t pin);
  ```
  Set the GPIO Input-Pin for the DRDYn input, internaly it will be set as an input with pullup

  ```cpp
    bool isAcquisitionReady();
  ```
  Returns false if the pin is not set or the DRDYn-Pin is high



## Heater details
See HDC1010 and HDC1080 Datasheet  
> 8.3.3 Heater
> The heater is an integrated resistive element that can be used to test the sensor or to drive condensation off the
> sensor. The heater can be activated using HEAT, bit 13 in Configuration Register. The heater helps in reducing
> the accumulated offset after long exposure at high humidity conditions.
> Once enabled the heater is turned on only in the measurement mode. To have a reasonable increase of the
> temperature it is suggested to increase the measurement data rate.


## What's Next

- Write tests
- Add examples
