## Data Ferry Usage

1. [Raspbian OS Installation](/data_ferry/system_install/README.md)
2. [Software and Dependencies Installation](/data_ferry/software_installation/README.md)
3. [Network Configuration and Component Servers](/data_ferry/network_configuration/README.md)
4. [PKI_configuration](/data_ferry/PKI_configuration/README.md)
5. [Data Ferry Usage, Server Management, and Debugging](/data_ferry/server_management/README.md) <--You are here

If you have made it this far in the guide, we generally recommend rebooting the Data Ferry system to make sure all of the services start and are working.

Use the following commands to determine current running services of the Data Ferry:

```
sudo systemctl status droneserver.service
sudo systemctl status isc-dhcp-server.service
sudo systemctl status hostapd
sudo systemctl status chrony
sudo systemctl status caddy
sudo systemctl status systemd-networkd
```

- Any status other than **active (running)** need to be addressed before proceeding.

Upon successful configuration of your FoxNode(s) and Data Ferry, the FoxNode should automatically connect to the Data Ferry Wi-Fi network and the Data Ferry will collect and store data to the sqlite3 database. sqlite3 will automatically create the associated database files in the /home/pscr/data_ferry/ working directory.

Using your "Command Server" computer, set the device's Wi-Fi IP address as 192.168.50.10/16 as specified in [network architecture](/pics/Network_Schema_Example.png)

Open a web browser and enter https://192.168.50.20/ui/  
**NOTE:** It's likely you haven't imported your drone server's certificate information to your host PC, so it will likely give you an HTTPS error. Acknolege and/or "Proceed" if prompted.

From here you will see various options for settings and FoxNode collection stats.
![Data Ferry UI Home Page](/pics/UI_Homepage.png)

The Data Ferry UI includes a "Light Mode" option checkbox. "Dark Mode" is enabled by default. Check the "Light mode" box to enable Light Mode.
- Dark Mode
![Data Ferry UI Dark Mode](/pics/UI_Dark_Mode.png)
- Light Mode
![Data Ferry UI Light Mode](/pics/UI_Light_Mode.png)

The Data Ferry UI web page will show you data collected from connected FoxNodes and associated data values and generated graphs. Click the "View" button next to the desired FoxNode to show graph data.
![FoxNode Graph Data in Data Ferry UI](/pics/UI_Graphs.png)

The Data Ferry does not automatically archive or rotate data logs. Data must be cleared manually otherwise the database will become quite large if left unattended.
A "Clear Data" button is provided to purge the database. The "admin token" using the phrase "clearme" has to be entered to peform the purge.
![Data Ferry Clear Data Button](/pics/UI_Clearme.png)

A general system "health" link provides status of the Data Ferry system.
![Data Ferry Health](/pics/UI_Health.png)

All gathered data can be viewed from the "/dump/full" link. This shows all of the gathered FoxNode data as JSON. A "Pretty-print" checkbox will display JSON in human readable format.  
![FoxNode JSON Data](/pics/UI_Dump.png)

Scroll down the the end of the dump output to see information for "first seen" or "earliestTakeOff", "sortieCount" and others.
![Data Ferry Collection Stats](/pics/UI_Dump_2.png)

The "Map" button next the the "Clear data" button will open a new tab with the location of collected FoxNodes.
Note that the use of Google Maps requires a Google account and Google Maps API key. Setup and configuration of the API key and maps funtionalty is not covered in this instruction, but placeholders are present for integration.
![Data Ferry Maps](/pics/UI_Map.png)

At this point you should have a completely functional Data Ferry system. If you do not have functional FoxNode sensors we recommend proceeding to the [FoxNode](/foxnode/README.md) section.