**Step 1**: Install the latest version of Raspbian on your Pi; preference is given to the "lite" version OS without a GUI due to the limited compute resources of the Pi Zero. Default install provisioning and settings are sufficient. Disk encryption is recommended for applicaitons that require enhanced security.

- For the following steps, it is recommended to connect the Pi to the internet so that software and dependencies can be installed. You may use the hotspot method mentioned above or an available Wi-Fi network.
- In our examples, we created a user named "pscr". Change the username to any desired username during the system install.

**To-Do** Additional guidance will be provided for system installation specific to this project. For now, refer to [Raspberry Pi Documentation](https://www.raspberrypi.com/documentation/computers/getting-started.html) for installation instructions.

**NOTE**: For enhanced security it is recommended to use disk encrytion or LUKS during installation. It is recommended to enable auto boot for the installation utilizing an initramfs script or key file on a separate hidden partition or USB drive to manage automatic unlocking or remote unlocking. Final documentation will cover this topic in a future update.

Proceed to [Software and Dependencies Installation](/data_ferry/software_installation/README.md) once you have completed OS installation.