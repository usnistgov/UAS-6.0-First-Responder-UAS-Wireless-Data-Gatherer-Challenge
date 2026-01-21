# UAS 6.0 Stage 3 FoxNode Sensor
The following provides component list and instuction for building and programming the FoxNode sensor.
Note that individuals that intend to replicate the pcb companion board will need to source a pcb fabrication facility and associated board components.

- PSCR posesses a limited number of "complete" FoxNode sensors. If you wish to develop on this platform or want to borrow our FoxNode sensors for your experimentation, please contact psprizes@nist.gov

## FoxNode Folder Structure
├── foxnode/            	 <-- FoxNode project (Stage 3)
│   ├── 3D_print_case_models <-- gcode, 3mf and dxf files for 3d printing FoxNode cases
│   ├── libraries            <-- Arduino libraries required for Foxnode
│   ├── pcb_schematic        <-- Schematics for FoxNode I2C companion board
│   ├── sample_outputs		 <-- FoxNode serial output debugs
│   ├── stateMachine         <-- FoxNode code (bread and butter of the project)
│   └── README.md            <-- This page

## FoxNode Components
**Hardware**: 
- ESP32-S2 ([Adafruit ESP32-S2 TFT Feather](https://learn.adafruit.com/adafruit-esp32-s2-tft-feather/overview))
- [Battery pack](https://www.ravpower.com/products/ravpower-10000mah-power-bank-dual-outputs?_pos=1&_psq=prime+1000&_ss=e&_v=1.0) (USB-C out to ESP32)
- USB to USB-C cable (Battery --> ESP32)
- ESP32_I2C Companion Board Schematic [Schematic and Parts List](foxnode/pcb_schematic/)
- [100mm STEMMA QT / Qwiic JST-SH 4-pin cable](https://www.adafruit.com/product/4210)

**3D-Printed Cases**:
- [3D-Print Case Models](foxnode/3D_print_case_models/)
- All of the Gcode are sliced for a Prusa MK3s+ printer with a .4mm nozzle and stacked layers at .15 mm
- .3mf files are included if you choose to do your own slicing for your specific 3D printer or application
- .gcodes are included for both PLA and PETG filament types, specifically for the Prusa 3D Printer
- Any printer with a .4mm nozzle that uses 1.75mm filament shall work as long as it can accommodate .15mm tall layers
- .dfx CAD files are included for the fabrication of the case window, so the LCD is visible. You will have to source your own fabrication service or device.
- The window should be a .125” thick Lexan or other polycarbonate. The minimum hole size is .125”, so many of the on-line services can create the windows.
- All hardware is 20mm M2.5.  You’ll need 6 per chassis to put the chassis together, and two 8mm to hold in the window on the left side. The M2.5 nylon cap nuts should be used.

**Important Notes**:
- The ESP32 family of embedded microcontrollers has many different purchasing options that often use different/incompatible displays, bus connections, peripherals, etc. To avoid confusion, it is highly advisable to purchase the specific ESP32 "flavor" mentioned above for the best compatibility with the provided source code. 
-The I2C Companion Board schematics and [parts list](foxnode/pcb_schematic/esp32_I2C_Parts_List.csv) are provided in the [pcb_schematic](foxnode/pcb_schematic/) directory. 

**Software**: 
- Host PC or target to program FoxNode(s)
- [Arduino IDE](https://www.arduino.cc/en/software)
- [Arduino Libraries and dependencies](/foxnode/libraries)
- Optional - [KiCad](https://www.kicad.org/) For reading pcb and case CAD files
- Optional - Slicing software for your 3D printer or application 

Note that up-to-date Arduino libraries may also be downloaded via the Arduino IDE libraries interface. For quick deployment and compatibility, the required libaries have been included in this repository. 

## FoxNode Installation & Setup
- Download or clone Github repository to your target "host PC."
```
git clone https://github.com/usnistgov/UAS-6.0-First-Responder-UAS-Wireless-Data-Gatherer-Challenge.git
```
- Install the [Arduino IDE ](https://www.arduino.cc/en/software) on the target to build project source code and program ESP32 hardware.
- ESP32 hardware support package dependencies listed below, install via Arduino IDE GUI "[Board Manager](https://docs.arduino.cc/software/ide-v2/tutorials/ide-v2-board-manager/)" 
    - [esp32](https://github.com/espressif/arduino-esp32) by Espressif Systems <-- main "Arduino-esp32" wrapped software stack, search Espressif Systems in the "Boards Manager", select and install.
	- Open [stateMachine.ino](foxnode/stateMachine/stateMachine.ino) file, confirm/acknowlege the creation of "stateMachine" directory.
	- Move/Copy [libraries](foxnode/libraries) directory and contents from the cloned repository to the "Arduino" directory in your home directory.

Please note that the specific path for your cloned GitHub repository and Arduino libraries may differ.
Windows: 
```
robocopy "C:\Users\%USERNAME%\Documents\GitHub\UAS-6.0-First-Responder-UAS-Wireless-Data-Gatherer-Challenge\Stage3\libraries" ^
         "C:\Users\%USERNAME%\Documents\Arduino\libraries" ^
         /E
```
macOS: 
```
rsync -av \
"$HOME/Documents/GitHub/UAS-6.0-First-Responder-UAS-Wireless-Data-Gatherer-Challenge/Stage3/libraries/" \
"$HOME/Documents/Arduino/libraries/"
```
Linux: 
```
rsync -av \
"$HOME/GitHub/UAS-6.0-First-Responder-UAS-Wireless-Data-Gatherer-Challenge/Stage3/libraries/" \
"$HOME/Arduino/libraries/"
```

## FoxNode Configuration Variables
The FoxNode must be configured for your specific network architecture and it is recommended to change Wi-Fi SSID passwords before proceeding. The network schema can be viewed [here]( (pics/Network_Schema_Example.png). Pre-populated values in this repository are not not secure, are considered compromised, and should not be used.

- FoxNode network configuration is found in the [httpComms.h](foxnode/stateMachine/httpComms.h) file. Edit the file cloned in your local copy of the repository.

Open the httpComms.h file in your favorite text editor or Arduino IDE and modify the following values to match your environment. The following values are prepopulated:
```
// Global Variables
//// UAS Server Target vars
#define WIFI_SSID     "uas6"			    // target network for  UAS 6.0 Prize challenge
#define WIFI_PASSWORD "hello123"			// target network for  UAS 6.0 Prize challenge
#define UAS_Server "http://192.168.40.20"	// Target Data Ferry or Drone

#define UAS_Gateway "192,168,40,20"			// Gateway for Sensor Client
#define UAS_DNS		  "8,8,8,8"				// DNS for Wifi
#define UAS_Subnet	"255,255,0,0"

// for loating up the Real Time Clock RTC from a hotspot on the net
#define NTP_WIFI_SSID     "UAS_NTP"				// target network for  NTP (external WiFi Hotspot-system setup only)
#define NTP_WIFI_PASSWORD "whatTime!"			// target network for  NTP
```
Note that hardcoded passwords are not recommended for production use. The following methods are recommended for further development and device hardening:
- Use ESP32 NVS encryption / secure boot / flash encryption (Recommended)
- Enterprise Wi-Fi (WPA2-Enterprise / WPA3-Enterprise) certificate/device based authentication (Recommended)
- Provision at runtime and store in non-volatile memory. Requires manual setup on each boot.
- Move secrets out of code using build-time secrets (for repository and dev environments)

The FoxNode ID must be set and unique for each FoxNode. The FoxNode IP address is automatically formulated using this following function call in httpComms.cpp.
```
IPAddress getFoxNodeIP(unsigned short thisFoxNodeId){		// formulate IP address based on Fox-Node ID
	if(thisFoxNodeId > 60)thisFoxNodeId = 5;				      // This number is the FoxNode ID and must be unique to each FoxNode
	return IPAddress(192, 168, 40, thisFoxNodeId + 80);   // This adds the FoxNode ID is to 80 as the IP address. For example, this will produce an IP of 192.168.40.85 for FoxNode 5.
```

With the IDE setup completed and variables configured, the "FoxNode to be" (ESP32 hardware) can now be connected to the host PC via USB and programmed with the provided source code file via Arduino IDE. 

# FoxNode Operation
1. The FoxNode sensor will first attempt to auto-connect to the predefined WiFi credentials to obtain NTP clock. In testing a phone with LTE/5G internet access running a Wi-Fi hotspot was used for this purpose, using the "UAS_NTP" credentials outlined in the previous section. 
2. NTP is optional and the sensor will automatically proceed to the next connection phase if it is unable to connect to an NTP server, but will "freewheel" with incorrect timestamps.
3. Next the FoxNode will attempt to connect to the Wi-Fi network defined in WIFI_SSID
4. Once connected the FoxNode will send HTTP-POST requests to the URI defined in UAS_Server
5. FoxNode will continue to send HTTP-POST updates every 10 seconds

- See Appendix B and C of the [UAS_6.0_Stage_3_Guidance](https://github.com/usnistgov/UAS-6.0-First-Responder-UAS-Wireless-Data-Gatherer-Challenge/tree/main/docs/UAS_6.0_Stage_3_Guidance.pdf) for full FoxNode state machine operation.

FoxNode verification:
The Arduino IDE provides easy access to a serial interface connection (115200 baud) that displays runtime information. This information is also pushed/displayed via an ESP TFT display (unique to the Adafruit ESP32-S2 TFT Feather). 

Upon connection to "UAS6" WiFi Network serial output... ex: 

```
WiFi connected
**FoxNode Initialized**
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

If the FoxNode fails to find the specified WiFi Network... ex:

```
**FoxNode Initialized**
WiFi Station: 14 Reason: 201
Trying to Reconnect
WiFi Station: 14 Reason: 201
Trying to Reconnect
```

NOTE: not all ESP32 hardware will have a display output. To make this project compatible with such hardware all references to the TFT display can be removed from the source Foxclient_HTTP.ino file to provide more "generic" ESP32 flavor compatibility.

## ESP32 Display and Operational States

FoxNode display states:

<Disp. # >, description (background color)

- 3, NTP Connection at powerup/boot,                   (Yellow)

![FoxNode NTP Connection](pics/FoxNode_NTP_Connect.jpg)

- 2, Not connected to Data Ferry or searching,   (Orange)

![FoxNode Searching for Data Ferry Network](pics/FoxNode_Searching.jpg)

- 1, On WiFi got IP handshaking for data exchange,                 (Blue)

![FoxNode Connected to Data Ferry, no Data Transfer](pics/FoxNode_WiFi_Connect.jpg)

- 0, Fully connected to Data Ferry and exchaning data,                  (Black)

![FoxNode Fully Connected, Data Exchange](pics/FoxNode_Fully_Connected.jpg) 