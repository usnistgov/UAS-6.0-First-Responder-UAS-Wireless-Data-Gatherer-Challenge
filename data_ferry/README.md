# Data Ferry (Drone Server) and Command Server
The Data Ferry presented in this section was used for proof-of-concept (lab) testing. The Data Ferry has a functional web server so that a PC client can view data collected and processed by the Data Ferry. The Data Ferry is a Python Flask server with Javascript front end and uses sqlite3 for data storage and retrieval. For the purposes of testing the "command server" data post-processing functionality is performed on the Data Ferry. The PC client is only used to view the data.

The network schema can be viewed [here]( (pics/Network_Schema_Example.png). Pre-populated values in this repository are not not secure, are considered compromised, and should not be used.

- Where cited portions of the Data Ferry code was generated with assistance from ChatGPT (OpenAI) and Gemini (Google). The code has been reviewed and integrated by the project authors.
- Data Ferry configuration is system dependent; however, we have provided a hardware list, configuration, and provisioning parameters in the following sections for reference if you are trying to replicate our setup.

## FoxNode Folder Structure
├── data_ferry/            	 <-- Data Ferry (Drone Server) project
│   ├── raspbian_files       <-- Raspbian configuration files and prebuilt ISO image
│   ├── static               <-- Javascript app and css for web server
│   ├── templates		     <-- HTML templates for web server
│   ├── app.py               <-- Python Flask web app
│   ├── README.md            <-- This page
│   └── requirements.txt     <-- Python requirements

## Data Ferry Hardware Components
- Target PC used to "view" web pages on Data Ferry. Any OS flavor as long as it has a web browser and you are able to manually change and set network configuration settings. A laptop or portable computer is also preferred if you plan to field test the Data Ferry on a drone.

- Raspberry Pi or lightweight compute platform that can be mounted to a drone. In PSCR's testing we used a [Raspberry Pi Zero 2 W](https://www.raspberrypi.com/products/raspberry-pi-zero-2-w/).

- For a portable power source for Raspberri Pi, we used the [PiSugar 1200 mAh Pi Zero Battery](https://docs.pisugar.com/docs/product-wiki/battery/pisugar2/pisugar-2). The PiSugar 2 Wiki support page includes power management software as well as 3D Print cases that will house both the Pi, PSugar module and battery. Installation instructions can be found in the Wiki link above. [PiSugar 2 3D Printer files](https://github.com/PiSugar/PiSugar) are available in the provided link.

- We used a high gain USB Wi-Fi adaptor for the Raspberry Pi and/or Target PC. We used the [Netgear A6210 High Gain WiFi USB Adapter](https://www.netgear.com/support/product/a6210). This is plug and play an no addtional drivers are required.

- A [small USB hub with a micro-USB male end](https://www.adafruit.com/product/2991) to connect to the Pi and USB-A ports for other peripherals, including the Wi-Fi adaptor.

- Optional, a short male micro-USB to female USB-A connector. This is good for when you want to eliminate the USB hub and just plug in the Wi-Fi adaptor.

- Optional (Recommended), mini-HDMI to HDMI cable, keyboard, mouse, and monitor to configure your Pi.

Note that when using a USB hub the Pi assigns an "ID" to the USB peripheral, so moving a device between different hub ports or to a port directly to the Pi will assign it a different ID, so any provisioning for and external Wi-Fi adaptor will have to be updated to reflect it's new port assignment.

Prototype Example of Data Ferry:
![Prototype Data Ferry](pics/Data_Ferry_Prototype.png)

## Data Ferry Configuration (Raspberry Pi Zero 2 W)
- In our setup we disabled NetworkManager in favor of systemd-networkd and netplan.

- The Pi is set up in Access Point (AP) Mode. The integrated Wi-Fi in the Pi Zero does not support AP mode. In addition to needing a higer-gain antenna, we found it best to use an external USB Wi-Fi adaptor that supported AP mode. External Wi-Fi adaptors typically show up as wlan1.

- Configure wlan0, integrated Wi-Fi, as a client to connect to the UAS_NTP hotspot or other internet connection.

- For the AP software we used hostapd

- dnsmask is installed for FoxNodes or devices that use DHCP

- Optional - install iptables for device network security (configuration for iptables is not provided in this repository)

- Optional - enable IP routing if you want the Pi to act as a router (configuration not provided)

1. Install the latest version of Raspbian to your Pi, preference is given to the "lite" version OS without a GUI due to limited compute resources of the Pi Zero. Default install provisioning and settings are sufficient.

- For the following steps it is recommended to connect the Pi to the internet so that software and dependencies can be installed. You may use the hotspot method mentioned above, or use an available Wi-Fi network.
- In our examples we created a user named "pscr". Change the username to any desired username during the system install.

2. Update Raspberry Pi and install software dependencies
```
sudo apt-get update && sudo apt-get upgrade -y
sudo apt-get install netplan.io systemd systemd-resolved hostapd dnsmasq iptables iptables-persistent python3-full git
```

3. Change to your user's home directory and download/clone GitHub repository to the Pi
```
cd ~
git clone https://github.com/usnistgov/UAS-6.0-First-Responder-UAS-Wireless-Data-Gatherer-Challenge.git
```

4. Create a copy of the data_ferry code to a separate working directory.
- We recommended creating a copy of the data ferry to another working directory. This helps streamline installation by matching paths in configuraiton files and reducing path length.
```
sudo cp -R ~/UAS-6.0-First-Responder-UAS-Wireless-Data-Gatherer-Challenge/data_ferry/* ~/data_ferry/
```
Note: For the remaining instructions we presume that you cloned the repository to your user's home directory and created a copy of the data_ferry directory
```
/home/pscr/data_ferry
or
~/data_ferry
```
Adjust paths according to your deployment.

5. Install Python Flask
Note: For this project we are using a dedicated device and install for a single Python app. If you plan to run other Python projects on your device it is recommended to use Python virtual environments (venv). For this project we are choosing to "break system packages." Acknowlege/confirm any warnings against doing so.
```
sudo /bin/python pip install flask --break-system-packages
```

5. Plug in external USB Wi-Fi adaptor and determine what interface it is assigned.
```
sudo ip link show
```
and/or
```
sudo iwconfig
```
- Typically the new interface shows up as wlan1, but you may have to run the command(s) before and after you plug it in.

3. Stop and disable NetworkManager
```
sudo systemctl stop NetworkManager
sudo systemctl disable NetworkManager
```
4. Create new network configuration .yaml file or copy [dataferry.yaml](data_ferry/raspbian_files/dataferry.yaml) from respository to /etc/netplan/
```
sudo cp ~/data_ferry/dataferry.yaml /etc/netplan/dataferry.yaml
```

6. Apply network configuration
```
sudo netplan apply
```

7. Configure the Pi to be an access point
- Disable RFkill so that you can run in AP mode
```
sudo /usr/sbin/rfkill unblock wifi
```
- Create or copy [hostapd.conf](data_ferry/raspbian_files/hostapd.conf) to /etc/hostapd/hostapd.conf
```
sudo cp ~/data_ferry/hostapd.conf /etc/hostapd.conf
```
- Note change SSIDs and wpa_password to match your architecture.

8. Create/copy [rfkill startup service](data_ferry/raspbian_files/rfkill-unblock-wifi.service) that automatically disables RFKill on startup
Copy the file to the /etc/systemd/system directory
```
sudo cp ~/data_ferry/rfkill-unblock-wifi.service /etc/systemd/system/rfkill-unblock-wifi.service
```
- Enable the rfkill service to start at boot, and run service
```
sudo systemctl daemon-reload
sudo systemctl enable rfkill-unblock-wifi.service
sudo systemctl start rfkill-unblock-wifi.service
```

8. Optionally configure/copy [/etc/dnsmasq.conf](data_ferry/raspbian_files/dnsmasq.conf)
- Change parameters to match your network schema or copy file from repositiory
```
sudo cp ~/data_ferry/rfkill-unblock-wifi.service /etc/dnsmasq.conf
```

9. Enable, start/restart services
```
sudo systemctl enable dnsmasq
sudo systemctl start dnsmasq
sudo systemctl unmask hostapd
sudo systemctl enable hostapd
sudo systemctl start hostapd
sudo systemctl restart systemd-networkd
```

11. Configure the Pi to start the Data Ferry automatically on startup by creating a startup service

- Optional - Add you Google Maps API key in the placeholder value <your API key> using the command below. Replace "AIzaSyBwV_B_B_B_B_B_B_B_B_B_B_B_B" in the command below with your actual key. It will still work without it or if left unchanged.
```
sed -i "s/<your API key>/AIzaSyBwV_B_B_B_B_B_B_B_B_B_B_B_B/g" ~/data_ferry/raspbian_files/droneserver.service
```

- Optional - Edit the service file to use your user's name if you changed it from pscr, using the command below:
```
sed -i "s/pscr/$USER/g" ~/data_ferry/raspbian_files/droneserver.service
```

- Next copy the file to the /etc/systemd/system directory
```
sudo cp ~/data_ferry/droneserver.service /etc/systemd/system/droneserver.service
```

- Enable the droneserver service to start at boot, and run service
```
sudo systemctl daemon-reload
sudo systemctl enable droneserver.service
```

- Start service immediately without reboot
```
sudo systemctl start droneserver.service
```

12. Optional - Enable ssh for remote administration
```
sudo systemctl enable ssh
sudo systemctl start ssh
```

## Data Ferry Usage
Upon successful configuration of your FoxNode(s) and Data Ferry, the FoxNode should automatically connect to the Data Ferry Wi-Fi network and the Data Ferry will collect and store data to the sqlite3 database. sqlite3 will automatically create the associated database files in the /home/pscr/data_ferry/ working directory.

Using your "Command Server" computer, set the device's Wi-Fi IP address as 192.168.50.10/16 as specified in [network architecture](pics/Network_Schema_Example.png)

Open a web browser and enter http://192.168.50.20/ui/
From here you will see various options for settings and FoxNode collection stats:
![Data Ferry UI Home Page](pics/UI_Homepage.png)

The Data Ferry UI includes a "Light Mode" option checkbox. "Dark Mode" is enabled by default. Check the "Light mode" box to enable Light Mode:
- Dark Mode
![Data Ferry UI Dark Mode](pics/Dark_Mode.png)
- Light Mode
![Data Ferry UI Light Mode](pics/Light_Mode.png)

The Data Ferry UI web page will show you data collected from connected FoxNodes and associated data values and generated graphs. Click the "View" button next to the desired FoxNode to show graph data.
![FoxNode Graph Data in Data Ferry UI](pics/UI_Graphs.png)

The Data Ferry does not automatically archive or rotate data logs. Data must be cleared manually otherwise the database will become quite large if left unattended.
A "Clear Data" button is provided to purge the database. The "admin token" using the phrase "clearme" has to be entered to peform the purge.
![Data Ferry Clear Data Button](pics/Clearme.png)

A general system "health" link provides status of the Data Ferry system:
![Data Ferry Health](pics/UI_Health.png)

All gathered data can be viewed from the "/dump/full" link. This shows all of the gathered FoxNode data as JSON. A "Pretty-print" checkbox will display JSON in human readable format:
![FoxNode JSON Data](pics/UI_Dump.png)

Scroll down the the end of the dump output to see information for "first seen" or "earliestTakeOff", "sortieCount" and others:
![Data Ferry Collection Stats](pics/UI_Dump_2.png)

The "Map" button next the the "Clear data" button will open a new tab with the location of collected FoxNodes.
Note that the use of Google Maps requires a Google account and Google Maps API key. Setup and configuration of the API key and maps funtionalty is not covered in this instruction, but placeholders are present for integration.
![Data Ferry Maps](pics/UI_Map.png)