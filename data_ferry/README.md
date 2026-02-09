# Data Ferry (Drone Server) and Command Server
The Data Ferry presented in this section is used for proof-of-concept (lab) testing. The Data Ferry has a functional web server so that a PC client can view collected data that is processed by the Data Ferry. The Data Ferry is a Python Flask server with a Javascript components and uses sqlite3 for data storage and retrieval. For the purposes of testing the "command server," data post-processing functionality is performed on the Data Ferry instead of a dedicated command server. The PC that would normally be used for the command server is only used to view the data though a web browser.

The network schema can be viewed [here](/pics/Network_Schema_Example.png).

**NOTE:** Pre-populated values such as SSIDs, pre-shared keys, certificates, and passwords presented in this repository are not secure, are considered compromised, and should not be used and are shown as examples or placeholders.  

**NOTE:** Where cited portions of the Data Ferry code was generated with assistance from ChatGPT (OpenAI) and Gemini (Google). The code has been reviewed and integrated by the project authors.

## FoxNode Repository Structure
```
├── data_ferry/                        # Drone Server / Command Server demo code
│   ├── 3D_print_case_models/		   # 3D Printer case files for Drone Server (To-Do)
│   ├── network_configuration/         # Configuration and apps to support network functions
│   ├── PKI_configuration/			   # Apps and support for mTLS and HTTPS
│   ├── server_app/					   # Drone server Python/Java application
│   ├── server_management/			   # GUI management and how-to
│   ├── software_installation/		   # Guide on what software to install on the Raspberry Pi
│   ├── system_install/			       # Guide on Raspberry Pi OS installation (To-Do)
│   └── README.md					   # This file
```

## Recommended Data Ferry Hardware Specifications

The following includes a list of recommended hardware components to build the data ferry server system. All instruction in this respository assumes the use of the exact hardware components listed here.

1. A "target PC" or "command server" used to "view" web pages on Data Ferry, perferm configuration tasks, and general debugging.
- Any OS flavor as long as it has a web browser, and you can manually change network configuration settings.
- A laptop or portable computer is also preferred if you plan to field test the Data Ferry on a drone.
- It is recommended to have a secure terminal application capable of initiating ssh sessions, such as Putty (Windows) or sshd (Linux and Mac)

2. Raspberry Pi or a lightweight compute platform that can be mounted to a drone. We use the [Raspberry Pi Zero 2 W](https://www.raspberrypi.com/products/raspberry-pi-zero-2-w/).

3. A portable power source for Raspberry Pi, we used the [PiSugar 1200 mAh Pi Zero Battery](https://docs.pisugar.com/docs/product-wiki/battery/pisugar2/pisugar-2). 
- The PiSugar 2 Wiki support page includes power management software as well as 3D-printed cases that will house the Pi, PSugar module, and battery.
- Installation instructions can be found in the Wiki link above. [PiSugar 2 3D Printer files](https://github.com/PiSugar/PiSugar) are available in the provided link. 
- The PiSugar 3 for Pi Zero will also work since it's the same formfactor, but you will have to make modifications to the 3D printer cases.

4. We used a high-gain USB Wi-Fi adaptor for the Raspberry Pi and/or Target PC.
- We used the [Netgear A6210 High Gain WiFi USB Adapter](https://www.netgear.com/support/product/a6210). This is plug-and-play, and no additional drivers are required.

5. A [small USB hub with a micro-USB male end](https://www.adafruit.com/product/2991) to connect to the Pi and USB-A ports for other peripherals, including the Wi-Fi adaptor.

6. Optional, a short male micro-USB to female USB-A connector. This is good for when you want to eliminate the USB hub and just plug in the Wi-Fi adaptor.

7. Optional (Recommended), mini-HDMI to HDMI cable, keyboard, and monitor to configure your Pi.

Prototype Example of Data Ferry:
![Prototype Data Ferry](<img src="/pics/Data_Ferry_Prototype.png" width="50%" />)

# Data Ferry system build and configuration

PSCR designed this section as an instructional step-by-step "build" to be followed in order to build the Data Ferry drone server.

1. [Raspbian OS Installation](/data_ferry/system_install/README.md) **To-Do**
2. [Software and Dependencies Installation](/data_ferry/software_installation/README.md)
3. [Network Configuration and Component Servers](/data_ferry/network_configuration/README.md)
4. [PKI_configuration](/data_ferry/PKI_configuration/README.md)
5. [Data Ferry Usage, Server Management, and Debugging](/data_ferry/server_management/README.md)
