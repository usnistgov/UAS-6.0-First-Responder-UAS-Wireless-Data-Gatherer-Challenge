# Data Ferry Network Configuration and Provisioning

1. [Raspbian OS Installation](/data_ferry/system_install/README.md)
2. [Software and Dependencies Installation](/data_ferry/software_installation/README.md)
3. [Network Configuration and Component Servers](/data_ferry/network_configuration/README.md) <--You are here
4. [PKI_configuration](/data_ferry/PKI_configuration/README.md)
5. [Data Ferry Usage, Server Management, and Debugging](/data_ferry/server_management/README.md) 

In this section we detail how to configure networking services, Wi-Fi AP mode or "hotspot", and create associated automatic startup services on boot.

## USB Device Paths and External Wi-Fi Adaptor

It is important to understand USB device paths in correlation with physical ports, and how these attributes determine the interface designation of your Wi-Fi Adaptor.

**Note about USB Device Paths:**
- When using a USB hub, the Pi OS assigns an "ID" to the USB peripheral, called the USB Device Path or USB Port Address.
- Moving a device between different hub ports or to a port directly to the Pi will assign it a different ID and therefore a different port designation.
- Any provisioning for an external Wi-Fi adaptor will subsequently have to be updated to reflect its new or current port assignment.

**For example:** Initially an external Wi-Fi adaptor is plugged into the first port of the USB Hub. The OS assigns it an id of "wlan1." After you finish the install, you remove the USB hub and plug the external Wi-Fi adaptor directly to the Pi. Even though it's the same port that the hub was plugged into, it now has a designation of "wlan2"
  
- Care must be taken to ensure that the WiFi adaptor is consistently connected to the same physical port, rather it directly connected to the Pi or though a USB hub.

### Step 1: Plug in the external USB Wi-Fi adaptor and determine what interface it is assigned.
```
sudo ip link show
```
OUTPUT
```
pscr@dronesvr:~ $ sudo ip link show
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN mode DEFAULT group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
2: wlan0: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc pfifo_fast state DOWN mode DORMANT group default qlen 1000
    link/ether d8:3a:dd:fe:7a:94 brd ff:ff:ff:ff:ff:ff
3: wlan1: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP mode DORMANT group default qlen 1000
    link/ether 8c:3b:ad:bb:b6:89 brd ff:ff:ff:ff:ff:ff
```

and/or

```
sudo iwconfig
```
OUTPUT
```
pscr@dronesvr:~ $ sudo iwconfig
lo        no wireless extensions.

wlan0     IEEE 802.11  ESSID:off/any
          Mode:Managed  Access Point: Not-Associated   Tx-Power=31 dBm
          Retry short limit:7   RTS thr:off   Fragment thr:off
          Encryption key:off
          Power Management:on

wlan1     IEEE 802.11  Mode:Master  Tx-Power=18 dBm
          Retry short limit:7   RTS thr:off   Fragment thr:off
          Power Management:off
```

- Typically the new interface shows up as **wlan1**, but you may have to run the command(s) before and after you plug it in to determine the interface name designation.

Here are some helpful commands that can be used in debugging to determine the physical USB address to the wlan configuration of your devices:

List usb devices:
```
sudo lsusb
```
OUTPUT
```
pscr@dronesvr:~ $ sudo lsusb
Bus 001 Device 004: ID 0846:9053 NetGear, Inc. A6210
Bus 001 Device 003: ID 3151:5026  2.4G Wireless Keyboard
Bus 001 Device 002: ID 214b:7250 Huasheng Electronics USB2.0 HUB
Bus 001 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
```

In the above lsusb output we notice that the OS detects a four port USB Hub connected to Port 1 and Port 4 contains our USB Wi-Fi adaptor. This can be determined by the "NetGear, Inc. A6219" designation.

We can correlate the "A6219" information by using the following command:

```
cat /sys/class/net/wlan1/device/interface
```
OUTPUT
```
pscr@dronesvr:~ $ sudo cat /sys/class/net/wlan1/device/interface
A6210
```

## Netplan Configuration

This section describes how to disable the default Network Manager and enable Netplan

### Step 2: Stop and disable NetworkManager service

Instead of uninstalling NetworkManager, we will instead disable it. This is useful if you ever need to reenable it at a later time.
```
sudo systemctl stop NetworkManager
sudo systemctl disable NetworkManager
```

### Step 3: Create or copy a new network configuration

Create or copy the .yaml file from [dataferry.yaml](/data_ferry/network_configuration/dataferry.yaml) to /etc/netplan/

```
sudo cp ~/data_ferry/dataferry.yaml /etc/netplan/dataferry.yaml
sudo chmod 600 /etc/netplan/dataferry.yaml
```

**NOTE**: You may need to edit the wlan interface designation if yours differs from wlan1 as discussed in the previous step. .yaml files are white space delimited and will prompt an error if not properly formatted.

### Step 4: Apply network configuration

```
sudo netplan apply
```

**NOTE:** Netplan may warn "Cannot call openvswitch..." ignore this warning message as it does not impact our server setup.

## Access Point or "Hotspot" configuration

In this section we will configure the new USB Wi-Fi adaptor as an access point or "AP" mode.

### Step 5: Disable RFkill so that you can run in AP mode

RFKill is a Linux kernel subsystem designed to disable radio transmitters to save power or comply with safety regulations (like airplane mode). It prevents you from running in Access Point (AP) mode because its primary function is to cut power to the physical radio, making the hardware unavailable to access point programs like hostapd. We need to deactivate this function to run in AP mode.

```
sudo /usr/sbin/rfkill unblock wifi
```

Using the rfkill commmand does not persist through reboots, but is good for testing to see if rfkill is blocking your interface.
In order to persist through reboot we need to create a .service file and tell systemd that we want to run it every time on boot.

Copy [rfkill startup service](/data_ferry/network_configuration/rfkill-unblock-wifi.service) to the /etc/systemd/system directory
```
sudo cp ~/data_ferry/rfkill-unblock-wifi.service /etc/systemd/system/rfkill-unblock-wifi.service
```

To enable the rfkill service to start at boot, and run service issue the following commands:
```
sudo systemctl daemon-reload
sudo systemctl enable rfkill-unblock-wifi.service
sudo systemctl start rfkill-unblock-wifi.service
```

### Step 6: Configure hostapd

Hostapd is the AP management software that allows our network inteface to function as a Wi-Fi access point.

Create or copy [hostapd.conf](/data_ferry/network_configuration/hostapd.conf) to /etc/hostapd/hostapd.conf
```
sudo cp ~/data_ferry/hostapd.conf /etc/hostapd.conf
sudo chmod 600 /etc/hostapd.conf
```
**NOTE:** Change SSIDs and wpa_password to match your architecture.

### Step 7: Configure our Pi as an DHCP server

Copy DHCP Server [dhcpd.conf](/data_ferry/network_configuration/dhcpd.conf)  
**NOTE:** Change parameters to match your network schema or copy file from repositiory
```
sudo cp ~/data_ferry/raspbian_files/dhcpd.conf /etc/dhcp/dhcpd.conf
```

### Step 8: Configure Data Ferry to be NTP server  

Copy chrony configuration file from repository to system directory  
**NOTE:** Change parameters to match your network schema or copy file from repositiory

```
sudo cp ~data_ferry/network_configuration/chrony.conf /etc/chrony/chrony.conf
sudo chmod 600 /etc/chrony/chrony.conf
```

### Step 9: Enable, start/restart services

Before we preceed further, we need to enable, start and restart services. The following commands will: 
- Enable the DHCP server at boot and start the service.
- Unblock hostapd at boot because the Linux kernel defaults to a "safe" state where radio transmitters are soft-blocked to conserve power or meet regulatory requirements, similar to rfkill. hostapd cannot initialize the radio if this block is active. Since we have created a startup service that unblocks wifi, this is safe to "unmask" and enable at boot.
- Restart chrony, droneserver and networking services

```
sudo systemctl enable isc-dhcp-server.service
sudo systemctl start isc-dhcp-server.service
sudo systemctl unmask hostapd
sudo systemctl enable hostapd
sudo systemctl start hostapd
sudo systemctl enable chrony
sudo systemctl start chrony
sudo systemctl restart systemd-networkd
sudo systemctl restart droneserver.service
```

## Next Step
Proceed to secure communications configuration for our HTTP server in [PKI_configuration](/data_ferry/PKI_configuration/README.md).