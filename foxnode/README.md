# UAS 6.0 Post Competition FoxNode Sensor
The following provides component list and instuction for building and programming the FoxNode sensor.
Note that individuals that intend to replicate the pcb companion board will need to source a pcb fabrication facility and associated board components.
Instruction is not provided on pcb fabrication, but details can be found in [parts list](/foxnode/pcb_schematic/esp32_I2C_Parts_List.csv) and [pcb_schematic](/foxnode/pcb_schematic/)

**NOTE:** PSCR posesses a limited number of "complete" FoxNode sensors. If you wish to develop on this platform or want to borrow our FoxNode sensors for your experimentation, please contact psprizes@nist.gov

## FoxNode Folder Structure
```
├── foxnode/            	 <-- FoxNode project (Stage 3)
│   ├── 3D_print_case_models <-- gcode, 3mf and dxf files for 3d printing FoxNode cases
│   ├── libraries            <-- Arduino libraries required for Foxnode
│   ├── pcb_schematic        <-- Schematics for FoxNode I2C companion board
│   ├── provision_secrets    <-- FoxNode secrets for NVM storage
│   ├── sample_outputs		 <-- FoxNode JSON outputs
│   ├── stateMachine         <-- FoxNode code (bread and butter of the project)
│   └── README.md            <-- This page
```

## FoxNode Components
**Hardware**: 
- ESP32-S2 ([Adafruit ESP32-S2 TFT Feather](https://learn.adafruit.com/adafruit-esp32-s2-tft-feather/overview))
- Power Bank (optional, but you can use any power bank or source) - [Anker PowerCore Slim 10000 Model A1229](https://www.anker.com/products/a1229) (USB-C out to ESP32)
- USB to USB-C cable (Power source --> ESP32)
- ESP32_I2C Companion Board Schematic [Schematic and Parts List](/foxnode/pcb_schematic/)
- [100mm STEMMA QT / Qwiic JST-SH 4-pin cable](https://www.adafruit.com/product/4210)

**3D-Printed Cases**:
- [3D-Print Case Models](/foxnode/3D_print_case_models/)
- All of the Gcode are sliced for a Prusa MK3s+ printer with a .4mm nozzle and stacked layers at .15 mm (common for most late model 3D printers as of 2026)
- .3mf files are included if you choose to do your own slicing for your specific 3D printer or application
- .gcodes are included for both PLA and PETG filament types, specifically for the Prusa 3D Printer
- Any printer with a .4mm nozzle that uses 1.75mm filament shall work as long as it can accommodate .15mm tall layers
- .dfx CAD files are included for the fabrication of the case window, so the LCD is visible. You will have to source your own fabrication service or device.
- The window should be a .125” thick Lexan or other polycarbonate. The minimum hole size is .125”, so many of the on-line services can create the windows.
- All hardware is 20mm M2.5.  You’ll need 6 per chassis to put the chassis together, and two 8mm to hold in the window on the left side. The M2.5 nylon cap nuts should be used.

**Important Notes**:
- The ESP32 family of embedded microcontrollers has many different purchasing options that often use different/incompatible displays, bus connections, peripherals, etc. To avoid confusion, it is highly advisable to purchase the specific ESP32 "flavor" mentioned above for the best compatibility with the provided source code. 
- The I2C Companion Board schematics and [parts list](/foxnode/pcb_schematic/esp32_I2C_Parts_List.csv) are provided in the [pcb_schematic](/foxnode/pcb_schematic/) directory. 

**Software**: 
- Host PC or target to program FoxNode(s), laptop recommended if you are taking it outside
- [Arduino IDE](https://www.arduino.cc/en/software)
- [Arduino Libraries and dependencies](/foxnode/libraries)
- Drone Server. If you need to provision a drone server, please see our build guide [here](/data_ferry/README.md)
- Optional - [KiCad](https://www.kicad.org/) For reading pcb and case CAD files
- Optional - Slicing software for your 3D printer or application, e.g. Bambu Studio, Prusa Slicer

**NOTE:** Up-to-date or current Arduino libraries may also be downloaded via the Arduino IDE libraries interface. For quick deployment and compatibility, the required libaries have been included in this repository, but does not ensure future compatibility, software bug patches, or security patches.

## FoxNode Installation & Setup

**Prerequisites:**
It is highly recommended to have a functional [data ferry (drone server)](/data_ferry/README.md) before provisioning your FoxNode(s). 

**Step 1:** Download repository to local host

Download or clone the UAS 6.0 Github repository to your target "host PC"

```
git clone https://github.com/usnistgov/UAS-6.0-First-Responder-UAS-Wireless-Data-Gatherer-Challenge.git
```

**Step 2:** Install Arduino IDE, ESP32 drivers, and project libraries

- Install the [Arduino IDE ](https://www.arduino.cc/en/software) on the target to build project source code and program ESP32 hardware.  

- ESP32 hardware support package dependencies listed below, install via Arduino IDE GUI "[Board Manager](https://docs.arduino.cc/software/ide-v2/tutorials/ide-v2-board-manager/)"   

- Install ExpressIf Software: [esp32](https://github.com/espressif/arduino-esp32) by Espressif Systems -- main "Arduino-esp32" wrapped software stack, search Espressif Systems in the "Boards Manager", select and install.  
	
- Open [stateMachine.ino](/foxnode/stateMachine/stateMachine.ino) file, confirm/acknowlege the creation of "stateMachine" directory.

**NOTE:** Ensure that the project support files "RingBuffer.cpp, httpComms.cpp, etc.", are in the same directory with stateMachine.ino. Typically when you confirm the creation of the directory it will create another directory inside of you current directory. This may incidently create a "nested" directory without the required Arduino C files. You will need to work from the newly created directory because that is where the Arduino IDE expects the files to be.

**Step 3:** Move/Copy [libraries](/foxnode/libraries) directory and contents from the cloned repository to the "Arduino" directory in your home directory.

The "libraries" file directory is different from the stateMachine one you created in the previous step. These libraries need to go into the Arduino IDE libraries directory. Alternatively, the libraries can be installed within the Arduino IDE; however, we have included them in this project for compatiblity and time savings. Note that the libraries included here will likely be out of date and not the most current version. Since this is more of an "archival" project, do not expect future updates that support updated libraries.

**NOTE:** The specific path for your cloned GitHub repository and Arduino libraries may differ depending on your system OS and environments. The following commands may be used with modification to match your environment:

Windows: 
```
robocopy "C:\Users\%USERNAME%\Documents\GitHub\UAS-6.0-First-Responder-UAS-Wireless-Data-Gatherer-Challenge\foxnode\libraries" ^
		 "C:\Users\%USERNAME%\Documents\Arduino\libraries" ^
		 /E
```
macOS: 
```
rsync -av \
"$HOME/Documents/GitHub/UAS-6.0-First-Responder-UAS-Wireless-Data-Gatherer-Challenge/foxnode/libraries/" \
"$HOME/Documents/Arduino/libraries/"
```
Linux: 
```
rsync -av \
"$HOME/GitHub/UAS-6.0-First-Responder-UAS-Wireless-Data-Gatherer-Challenge/foxnode/libraries/" \
"$HOME/Arduino/libraries/"
```

## FoxNode Configuration Variables
The FoxNode must be configured for your specific network architecture and it is recommended to change Wi-Fi SSID passwords before proceeding. The network schema can be viewed [here] (/pics/Network_Schema_Example.png). Pre-populated values in this repository are not not secure, are considered compromised, and should not be used.  

IP configuration of the FoxNode follows the following process:
![FoxNode IP Config](/pics/FN_Connection_Process.drawio.png)

- FoxNode network configuration is found in the [httpComms.cpp](/foxnode/stateMachine/httpComms.cpp) file and [httpComms.h](/foxnode/stateMachine/httpComms.h) files.

If you are replicating the uas6 FoxNode architecture, than you do not have to modify these files; however you will have to update the FoxNode ID for each of your FoxNodes.

The FoxNode ID must be unique for each FoxNode and has to be manually configured. The Static FoxNode IP address is formulated using this following function call in [httpComms.h](/foxnode/stateMachine/httpComms.h). This output is for informational purposes:

```
IPAddress getFoxNodeIP(unsigned short thisFoxNodeId){			// Formulate IP address based on Fox-Node ID
	if(thisFoxNodeId >= 175)thisFoxNodeId = 0;				    // This is a safety check to make sure we're not going over 255 when asigning FoxNodes
	return IPAddress(192, 168, 40, thisFoxNodeId + 80);   		// This adds the FoxNode ID plus 80 as the IP address.
```
The FoxNode ID is manually configured in the [eeprrom.cpp](/foxnode/stateMachine/eeprrom.cpp) file.  

**Step 4:** Change the FoxNodeId (repeated step for each foxnode)

**THIS VALUE MUST BE CHANGED FOR EACH FOXNODE**
```
void eepromSetVariablesToDefault(void){
	thisFoxNodeId = 10;                  // This is where you manually set the FoxNode ID, Do not assign values greater than 175.
...
}
```
- In the previous example, the FoxNode will be assigned a IP of 192.168.40.90.
- Even if you are using DHCP, this value still needs to be changed to uniquely identify your FoxNodes.

**NOTE:** Do not program your foxnode at this time. You will return to this after PKI configuration in the following steps.

## FoxNode Public Key Infastructure (PKI) Configuration and Wi-Fi Secrets

The FoxNode and Data Ferry (Drone Server) system uses mutual Transport Layer Security (mTLS) for both device authentication and HTTP payload encryption. Before starting this step you will need a Certificate Authority (CA) and a Certificate Server capable of creating client certificates and/or generating Certificate Signing Requests (CSR). If you are using this guide and creating your own drone server, this is covered in the [data ferry PKI Configuration](/data_ferry/PKI_configuration/README.md) section.

**Step 5:** 

Open and create the "provision_secrets.ino" Arduino project into the Arduino IDE.

Open [provision_secrets.ino](/foxnode/provision_secrets/provision_secrets.ino) file, confirm/acknowlege the creation of "stateMachine" directory.

**NOTE:** As before, ensure that the project support files "secrets.cpp", "secrets.h", and "serial.h" are in the same directory with provision_secrets.ino.

**Step 6:** Generate Client FoxNode Certificates

On your signing server, create client certificates. A separate certificate is required for each FoxNode in order to uniquely identify them. However, if you are using this in a test environment, you can use a single certificate that all of your FoxNodes use to save time (not recommended), but this defeats per-device identification and authentication.

**Step 7:** Copy Privacy Enhanced Mail (PEM) or X.509 information from your drone server

- Generation of the client certificates is cover in [data ferry PKI Configuration](/data_ferry/PKI_configuration/README.md)

You will copy three files from your certificate server to your host PC running Arduino IDE:
- ca/ca.crt - This is the CA certificate
- clients/foxnodeX.crt - This is the FoxNode client certificate
- clients/foxnodeX.key - This is the FoxNode Key

**Step 8:** Copy certificate information into provision_secrets.ino

- From Arduino IDE in the provision_secrets.ino project, make sure provision_secrets.ino is selected.  

Scroll down to the first block that shows:
```
-----BEGIN CERTIFICATE-----
```

- The first block should be within a constant varible called "PROV_CA_PEM."  
- Open the "ca.crt" file in a text editor (notepad++, notepad, etc.) and copy everything between "-----BEGIN CERTIFICATE-----" and "-----END CERTIFICATE-----"  
- Go back to Arduino IDE, provision_secrets.ino and paste the certificate information between the "BEGIN" and "END" delimiters.  

- Repeat these steps for "PROV_CLIENT_CERT" using the information in foxnodeX.crt and "PROV_CLIENT_KEY_PEM" using the information in foxnodeX.key.

**Step 9:** Modify or update Wi-Fi secrets (if changed)

- Scroll back up to before the first certificate block in provision_secrets.ino.  
- Locate the code section with the WiFi SSIDs and PSKs for both the NTP WiFi and UAS "operational network" networks.
- Confirm or update the SSID and PSK parameters according to your architecture.

**Step 10:** Program FoxNode with provision_secrets ino file

In this step you will "upload" the PEM certificate information to the FoxNode's Non-Volatile Memory (NVM) storage. Information stored in this part of memory will persist through reboots and subsequent programming.

- Once everything is copied in and confirmed, connect your PC to the FoxNode USB's port.
- Make sure that your ESP is selected in the COMs port selection dropdown.
- Click the "Upload" arrow button in the top right of the Arduino IDE.

- The "Output" screen should appear in the bottom part of the IDE, and will show the compilation and upload progress. Any compilation errors will appear here.

**NOTE:** The FoxNode LCD may will not show any relevant informaiton at this time.

**WARNING:** While the certificate and secrets are stored in a separate memory space, it is **NOT stored on encrypted media**; meaning, it can be retreaved if the physical device is compromised. For production devices it is recommended to encrypt PEM blocks in NVM and store a read-only decryption key within the ESP32's eFuse memory space. The eFuse memory space can only be written to once and never changed back. This configuration is not covered in this guide, but is recommended for production deployments. See more about eFuses [here](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/efuse.html)

With the certificate information in place, we can now proceed to programming the FoxNode stateMachine software.

**Step 11:** Program the stateMachine software

In step 4, you should have changed the FoxNode's ID to match your current node.  
- Reopen the Arduino IDE project containing the stateMachine project.  
- As before, make sure your FoxNode is selected in the COMs dropdown.  
- Next press the arrow "upload" button to compile and load the stateMachine software.

**NOTE:** This complete's the FoxNode programming portion. If you have additional FoxNodes to program, you will need to repeat steps 4 through 11 for each FoxNode.

# FoxNode Operation and Debugging  

During nominal operation, the FoxNode will operate as follows:

1. The FoxNode sensor will first attempt to auto-connect to the predefined WiFi credentials to obtain NTP clock. In testing, we used a phone with LTE/5G internet access running a Wi-Fi hotspot, using the "UAS_NTP" credentials outlined in the "PROV_NTP_SSID" constant variable in provision_secrets.ino.
2. NTP is optional and the sensor will automatically proceed to the next connection phase if it is unable to connect to an NTP server, but will "freewheel" with incorrect timestamps.
3. Next the FoxNode will attempt to connect to the "production" Wi-Fi network defined in "PROV_UAS6_SSID" within provision_secrets.ino
4. The FoxNode will first request DHCP network configuration from the Drone Server.
5. If the Drone Server does not have a DHCP server configured, the FoxNode will fallback to the hard coded static IP, based on it's FoxNode ID.
6. Once connected the FoxNode will send encrypted HTTPS, HTTP-POST requests to the URI defined in the "UAS_Server" variable the within "httpComms.h" file in the stateMachine Arduino project.
7. FoxNode will continue to send HTTP-POST updates every 10 seconds

- See Appendix B and C of the [UAS_6.0_Stage_3_Guidance](/docs/UAS_6.0_Stage_3_Guidance.pdf) for full FoxNode state machine operation.  
**NOTE:** Stage 3 Guidance does not support DHCP configuration or mTLS, only static addressing and unencrypted HTTP; however the state machine and order of operations remain the same as Stage 3.

**NOTE:** HTTPS mTLS authentication requires that the FoxNode and Drone Server's clock match, therefore it is crutial that NTP sync occur when the FoxNode boots up.

## FoxNode verification

The Arduino IDE provides easy access to a serial interface connection. Setting your serial speed to **230400 baud** displays relevant runtime information. Similar relevant information is also pushed/displayed in a minimal output via an ESP TFT display (unique to the Adafruit ESP32-S2 TFT Feather). From the Arduino IDE, Select Tools > Serial Monitor or Ctrl+Shift+M to display the serial monitor terminal. We have set the baud rate to a higher value of 230400, so not to miss any messages. Ensure that this baud rate is selected in the IDE dropdown in the terminal monitor section.

**NOTE:** Be aware of opening multiple Arduino "projects" and enabling serial monitoring on both instance. If multiple instances are open you may encounter "COM port busy" errors if you try to flash from another instance.

## Logging levels

While debugging messages are implemented throughout the FoxNode code, the ESP32 has limited buffer capabalities for debugging messages. Many debugging "prints" can be suppressed by setting the "serialLogLevel" variable in the [serial.cpp](/foxnode/stateMachine/serial.cpp) stateMachine code to prevent overloading the ESPs processor.  

The function for setting logging levels is located near the top of serial.cpp, along with serial port baud settings, and buffer sizes.
```
// Logging levels are:
//  0 - none      1 - errors   2 - general/high level info   3 - medium info   4 - 4 Very Verbose

void initializeSerial(void){
	serialLogLevel = 1;					// default out of boot is no logging, recommended level 1, 4 may risk overloading MCU
	Serial.begin(230400);			// Start Serial interface
	Serial.setRxBufferSize(2048); // Set buffer bigger for verbose logging
	delay(1000);						// for serial setup
	esp_reset_reason_t r = esp_reset_reason();
	Serial.printf("Reset reason: %d\n", r);
}
```

By default we have set the logging level to level 1, which is the lowest logging level. Logging levels can be adjusted to a more or less verbose output. In our observations, too much logging will often cause the serial monitor function to "freeze" and will no longer display debugging messages until the FoxNode is rebooted. If this occurs, you may have to reduce the logging level.

## Basic Logging Messages

**TO-DO:** This section does not currently reflect current code and needs updating, but follows very similar format.

With the FoxNode connected to your target PC, from the Arduino IDE make sure your ESP's COM port is selected in the top dropdown box, then select Tools > Serial Monitor
![Arduino Serial Monitor](/pics/ARD_Serial_Monitor.png)  

A successful FoxNode bootup will go throught the following processes:  

- FoxNode NTP Connect  
```
**FN-NTP RV-3032 Setup Initiated**
Entering init
Found RV3032
RTC init Success
RTC type = RV-3032
Set ESP time to RTC mem:
2026-1-22 17:52:28
Attempting NTP-WiFi Connection to SSID UAS_NTP
password iswhatTime!
....NTP Setup procTime=3765

NTP WiFi Connected
FN-NTP Time Set:175231
```
- FoxNode INIT, assigns ID set in httpComms.cpp in thisFoxNodeId  

```
**Fox-Node Init**
FoxNode ID: 4
**I2C**
Latched interrupt configured.
setup_webserver_routing() done
RingBuffer DataPayload Size: 106
Estimated max number of entries possible in Ringbuffer: 1093
Allocating 90 RingBuffer max entries.
```
- FoxNode Connecting to Drone Server  

```
WiFiInit() Starting WiFi
WiFiInit() Looking for SSID: uas6
WiFiInit()    with Password: hello123
WiFiStationConnected () - WiFi-STA Tx Power: 28
WiFiInit(): Exiting
```
- FoxNode POST "hi" and receive 'sval'  
```
takeSaveSensorData() Pushed data to RingBuffer, next sample in:30000 mS
MSM 0  Idle: Have connection to WiFi Net, going to state 1
MSM 1  POST hi: POSTing 'core' JSON to drone, ccmd='hi'
MSM 5  'hi' Post recived 'sval' reply: start TS=1073645108, end TS=1073645108
```
- FoxNode sending data entries  
```
Sending 1 Data Entries

MSM case 5: {"core":{"ccmd":"rval","fox":4,"fip":"192.168.40.84","fts":1769104354,"fbv":"F","fcnt":1,"ftso":1769104353,"ftsr":1769104353,"wrxs":-36,"lat":64.83,"lon":-147.77,"elev":137},"data":{"fox":4,"dlen":1,"ftso":1769104353,"dump":[{"rts":1769104353,"t":28.81483,"c":62.8,"h":41.70596,"l":423.68,"p":832.9751,"x":36,"y":-119,"z":928}]}}
MSM 5  POSTing core 'rval' JSON and 'data' to drone
```
- FoxNode recieve acknowlegement from Drone Server "buby" and send "bye" to close connection  
```
MSM 5  POSTing core 'rval' JSON and 'data' to drone
MSM 6  Recived POST from sending data, got drone srep'='buby', procTime=104
MSM 25  Recieved POST reply of 'buby', replying 'bye' to UAS Server
MSM 26  Recived 'bye' reply: got drone srep 'done', procTime=95 
** Transaction completed **
```
- FoxNode initiates wait timer till next transaction  
```
MSM 35: Going Blind
MSM 36: wifiDroneBlindTimer: 1500
MSM 36: wifiDroneBlindTimer: 0
MSM 36: Blind time expired, back to talking to Drone Server
```

Unsucessful FoxNode Attempts:  

-Failed to connect to NTP  
```
**FN-NTP RV-3032 Setup Initiated**
Entering init
Found RV3032
RTC init Success
RTC type = RV-3032
Set ESP time to RTC mem:
2026-1-22 20:58:28
Attempting NTP-WiFi Connection to SSID UAS_NTP
password iswhatTime!
........................................
Timeout reached, proceeding with regular code flow. NTP unable to set
```
- Failed to connect to Drone Server  
```
**Fox-Node Init**
FoxNode ID: 4
**I2C**
Latched interrupt configured.
setup_webserver_routing() done
RingBuffer DataPayload Size: 106
Estimated max number of entries possible in Ringbuffer: 1095
Allocating 90 RingBuffer max entries.
WiFiInit() Starting WiFi
WiFiInit() Looking for SSID: uas6
WiFiInit()    with Password: hello123
WiFiStationConnected () - WiFi-STA Tx Power: 28
WiFiInit(): Exiting

takeSaveSensorData() Pushed data to RingBuffer, next sample in:30000 mS
MSM Check: Attempting WiFi reconnect
MSM Check: Attempting WiFi reconnect
MSM Check: Attempting WiFi reconnect
MSM Check: Attempting WiFi reconnect
takeSaveSensorData() Pushed data to RingBuffer, next sample in:30000 mS
```

## FoxNode Display and Operational States

FoxNode display states:

- NTP Connection at powerup/boot (Yellow)

![FoxNode NTP Connection](/pics/FoxNode_NTP_Connect.jpg)

- Not connected to Data Ferry or searching for network (Orange)
Display shows connection status  
Drone Server SSID  
Drone Server PSK  
Timestamp  
Drone Server URI  

![FoxNode Searching for Data Ferry Network](/pics/FoxNode_Searching.jpg)

- Connected to Wi-Fi, set IP, handshaking for data exchange (Blue)
Wi-Fi Conection state  
Display shows Drone Server IP
RSSI reading  
Timestamp

![FoxNode Connected to Data Ferry, no Data Transfer](/pics/FoxNode_WiFi_Connect.jpg)

- Fully connected to Data Ferry and receiving data (Green)
Display show Timestamp  
RSSI reading    
Foxnode Transmit power (STA_TX_PWR)  
Drone Server IP (S)  
Drone Server HTTP Response Code (HTTP Resp)  
FoxNode IP (F)  

![FoxNode Fully Connected, Data Exchange](/pics/FoxNode_Fully_Connected_ALT.jpg) 

## Further Work

The following methods are recommended for further development and device hardening:
- Use eFuse for decryption key and store secrets on NVM encrypted (Recommended)
- Enterprise Wi-Fi (WPA2-Enterprise / WPA3-Enterprise) certificate/device based authentication. This can use the existing certs for mTLS. (Recommended)
- Secure coding audit. Code has not been reviewed for best practices.


