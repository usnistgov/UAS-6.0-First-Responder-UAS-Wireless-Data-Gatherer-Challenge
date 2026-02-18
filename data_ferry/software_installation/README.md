# Data Ferry (Drone Server) Software and Dependencies installation

This section outlines "manual" installation of the drone server software. It is assumed that [Part 1, Rasbpian OS Install](/data_ferry/system_install/README.md) is complete.

## Data Ferry Configuration Overview (Raspberry Pi Zero 2 W)

- In our setup, we disable NetworkManager in favor of systemd-networkd and netplan.

- The Pi is set up in **Access Point (AP) Mode**; however, the Pi Zero's integrated Wi-Fi does not support AP mode. We found it best to use an [external USB Wi-Fi adaptor](https://www.netgear.com/support/product/a6210) that supports AP mode and higher signal gain capabalities. External Wi-Fi adaptors typically show up as wlan1.

- Configure **wlan0**, integrated Wi-Fi, as a **client (STA)** to connect to the UAS_NTP hotspot or other internet connection. This is mostly for remote provisioning and downloading additional software.

- For the **AP software**, we used **hostapd**

- **isc-dhcp-server** is installed for FoxNodes or devices that use **DHCP**

- **caddy** is installed as a reverse proxy that supports **secure HTTP/TLS** in our application. Native TLS in Flask is not as extensible or secure, so caddy is used to handle secure data transfer.

- Optional - install **iptables firewall** for device network security (configuration for iptables is not provided in this repository)

- Optional - enable **IP routing** if you want the Pi to act as a router (configuration not provided)

## Software Instllation

**Step 1:** Update Raspberry Pi and install software servers, software, and dependencies.
```
sudo apt-get update && sudo apt-get upgrade -y
sudo apt-get install -y netplan.io systemd systemd-resolved hostapd dnsmasq iptables iptables-persistent python3-full git isc-dhcp-server tcpdump chrony
```

**Step 2:** Install Caddy reverse proxy
Install as follows:
```
sudo apt update
sudo apt install -y debian-keyring debian-archive-keyring apt-transport-https curl

curl -1sLf 'https://dl.cloudsmith.io/public/caddy/stable/gpg.key' \
  | sudo gpg --dearmor -o /usr/share/keyrings/caddy-stable-archive-keyring.gpg

curl -1sLf 'https://dl.cloudsmith.io/public/caddy/stable/debian.deb.txt' \
  | sudo tee /etc/apt/sources.list.d/caddy-stable.list

sudo apt update
sudo apt install -y caddy
```

**Step 3:** Change to your user's home directory and download/clone GitHub repository to the Pi
```
cd ~
git clone https://github.com/usnistgov/UAS-6.0-First-Responder-UAS-Wireless-Data-Gatherer-Challenge.git
```

**Step 4:** Create a copy of the data_ferry code to a separate working directory.

- This step helps streamline installation by matching paths in configuraiton files and reducing path length while preserving the "base" reference code.
```
cp -R ~/UAS-6.0-First-Responder-UAS-Wireless-Data-Gatherer-Challenge/data_ferry/* ~/data_ferry/
```
**Note:** For the remaining instructions we presume that you cloned the repository to your user's home directory and created a copy of the data_ferry directory
```
/home/pscr/data_ferry
or
~/data_ferry
```
Adjust paths according to your deployment.

**Step 5:** Install Python Flask

**Note:** For this project we are using a dedicated device and install for a single Python app. If you plan to run other Python projects on your device it is recommended to use Python virtual environments (venv). For this project we are choosing to "break system packages." Acknowlege/confirm any warnings against doing so.
```
sudo /bin/python pip install flask --break-system-packages
```

**Step 6:** Configure the Pi to start the Data Ferry automatically on startup by creating a startup service. For this step you will be modifying the [droneserver.service](/data_ferry/network_configuration/droneserver.service) file.

- Optional - Add your Google Maps API key in the placeholder value <your API key> using the command below. Replace "AIzaSyBwV_B_B_B_B_B_B_B_B_B_B_B_B" in the command below with your actual key. It will still work without it or if left unchanged.

```
sed -i "s/<your API key>/AIzaSyBwV_B_B_B_B_B_B_B_B_B_B_B_B/g" ~/data_ferry/raspbian_files/droneserver.service
```

- Optional - Edit the service file to use your user's name if you changed it from pscr, using the command below:

```
sed -i "s/pscr/$USER/g" ~/data_ferry/network_configuration/droneserver.service
```

- Next copy the file to the /etc/systemd/system directory

```
sudo cp ~/data_ferry/network_configuration/droneserver.service /etc/systemd/system/droneserver.service
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

**Step 7:** Optional - Enable ssh for remote administration
```
sudo systemctl enable ssh
sudo systemctl start ssh
```

Next proceed to [Network Configuration](/data_ferry/network_configuration/README.md)