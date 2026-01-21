# [UAS 6.0 First Responder UAS Wireless Data Gatherer Challenge Stage 2 FoxNode](https://www.nist.gov/ctl/pscr/open-innovation-prize-challenges/current-and-upcoming-prize-challenges/2024-first-responder)

## Scope
- This section contains the IoT-Client or "Sensor Module" reference software for the NIST-PSCR UAS6.0: First Responder UAS Data Gatherer Challenge "**Fox Hunt**" Stage 2.
- This section has been provided for archival reference and we recommend instead building the [Stage 3 FoxNode](foxnode) as it contains a more functional prototype.
- The Stage 2 FoxNode was a minimal viable prototype that used "canned" data. The Stage 3 FoxNode uses actual environmental data output and contains superior network handling functions.
- Stage 2 instructions and repository files have not been updated or modified and may contain errors or limited function. In other words, if you are new here, this is not where you should be.

**Stage 2**: Sensor Module addition to the [Guidance for Stage 2](docs/UAS_6.0_Stage_2_Guidance.pdf) document. See document for further information.

## Fox-Node Solution Components (Stage 2)
**Hardware**: 
- ESP32-S2 ([Adafruit ESP32-S2 TFT Feather](https://learn.adafruit.com/adafruit-esp32-s2-tft-feather/overview))
- [Battery pack](https://www.ravpower.com/products/ravpower-10000mah-power-bank-dual-outputs?_pos=1&_psq=prime+1000&_ss=e&_v=1.0) (USB-C out to ESP32)
- USB to USB-C cable (Battery --> ESP32)

NOTE: The ESP32 family of embedded microcontrollers has many different purchasing options that often use different/incompatible displays, bus connections, peripherals, etc. To avoid confusion, it is highly advisable to purchase the specific ESP32 "flavor" mentioned above for the best compatibility with the provided source code. 

**Software**: 
- Host PC to program target IoT Fox-Node(s)
    - [Arduino IDE ](https://www.arduino.cc/en/software)

## Installation & Setup
- Install the [Arduino IDE ](https://www.arduino.cc/en/software) on the target "host PC" to build project source code and program ESP32 hardware.
    - ESP32 hardware support package dependencies listed below, install via Arduino IDE GUI "[Board Manager](https://docs.arduino.cc/software/ide-v2/tutorials/ide-v2-board-manager/)" 
        - [esp32](https://github.com/espressif/arduino-esp32) by Espressif Systems <-- main "Arduino-esp32" wrapped software stack
    - Software dependencies libraries shown below, install via Arduino IDE GUI "[Library Manager](https://www.arduino.cc/en/Guide/Libraries/)" outside of the core Arduino-ESP32 software stack.
        - [ArduinoJson](https://github.com/bblanchon/ArduinoJson) v7.1.0 by Benoit Blanchon
        - [Adafruit ST7735 and ST7789 Library](https://github.com/adafruit/Adafruit-ST7735-Library) v1.10.4 by Adafruit (Select "Install All" option to bring in dependencies listed below)
            - Adafruit BusIO v1.16.1            (dependency of Adafruit ST7735 and ST7789 installed along side by default.)
            - Adafruit GFX Library v1.11.10     (dependency of Adafruit ST7735 and ST7789 installed along side by default.)
            - Adafruit seesaw v1.7.8 Library    (dependency of Adafruit ST7735 and ST7789 installed along side by default.)
            - SD v1.3.0 Library                 (dependency of Adafruit ST7735 and ST7789 installed along side by default.)

With the IDE setup completed, the "Fox-Node to be" (ESP32 hardware) can now be connected to the host PC via USB and programmed with the provided "Foxclient_HTTP.ino" source code file via Arduino IDE. 

# Usage
The "Fox Hunt" project leverages the [Arduino IDE](https://www.arduino.cc/en/software), [Arduino ESP32](https://github.com/espressif/arduino-esp32) software stack and [ESP32 hardware](https://learn.adafruit.com/adafruit-esp32-s2-tft-feather/overview) to run a simple HTTP-Client. This client (Foxclient_HTTP.ino), referred to as a "Fox-Node" will auto-connect to the predefined WiFi credentials and beacon an HTTP-POST message every 10 seconds.

NOTE: Target WiFi SSID/PSWD can be easily updated in the source. **Default SSID/PSWD** are both hardcoded as **"UAS6"**.

![Fox Node State Diagram](pics/FoxNode_StateDiagram.png)

Fox-Node verification:
The Arduino IDE provides easy access to a serial interface connection (115200 baud) that displays runtime information. This information is also pushed/displayed via an ESP TFT display (unique to the Adafruit ESP32-S2 TFT Feather). 

Upon connection to "UAS6" WiFi Network serial output... ex: 

```
WiFi connected
**Fox-Node Initialized**
WiFi Tx Power: 28
IP address: 
192.168.1.2
Attempting POST.
Server connected, Sending POST.
POST success.
STA PWR: 28
HTTP Response code: 200
POST Payload: {"Node_RSSI":-38}
```

If the Fox-Node fails to find the specified WiFi Network... ex:

```
**Fox-Node Initialized**
WiFi Station: 14 Reason: 201
Trying to Reconnect
WiFi Station: 14 Reason: 201
Trying to Reconnect
```

NOTE: not all ESP32 hardware will have a display output. To make this project compatible with such hardware all references to the TFT display can be removed from the source Foxclient_HTTP.ino file to provide more "generic" ESP32 flavor compatibility.

## ESP32 Display

Fox-Node display states:

<Disp. # >, description (background color)

- 3, on sent POST,                   (GREEN)

![Fox Node State Diagram](pics/FoxNode_disp3_tight.jpg)

- 2, no connection to HTTP server,   (Orange)

![Fox Node State Diagram](pics/FoxNode_disp2_tight.jpg)

- 1, On WiFi got IP,                 (Blue)

![Fox Node State Diagram](pics/FoxNode_disp1_tight.jpg)

- 0, TFT init code,                  (Green)

![Fox Node State Diagram](pics/FoxNode_disp0_tight.jpg)

