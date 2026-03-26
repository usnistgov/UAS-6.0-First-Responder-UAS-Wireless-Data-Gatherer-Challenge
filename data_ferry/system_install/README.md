# Data Ferry (Drone Server) System Installation Guide

This guide describes how to install and provision Raspberry Pi OS for the Data Ferry platform used in the UAS 6.0 project. It includes optional full-disk encryption guidance using LUKS.

1. [Raspbian OS Installation](/data_ferry/system_install/README.md) <--You are here
2. [Software and Dependencies Installation](/data_ferry/software_installation/README.md)
3. [Network Configuration and Component Servers](/data_ferry/network_configuration/README.md)
4. [PKI_configuration](/data_ferry/PKI_configuration/README.md)
5. [Data Ferry Usage, Server Management, and Debugging](/data_ferry/server_management/README.md)

> [!NOTE]
> Portions of this section was generated with the assistance of ChatGPT (OpenAI). Authors have reviewed and approved steps outlined in this section. Some steps and procedures may deviate depending on your specific hardware and OS version. For reference, this was last tested in February of 2026.

## 1. Hardware Requirements

Recommended platform:

-   Raspberry Pi Zero 2 W
-   32 GB or larger microSD card
-   5V power supply capable of supplying 2.5 Amps

Optional for secure deployments:

-   USB drive for encryption key storage
-   Secure storage location for passphrases and recovery keys

## 2. Install Raspberry Pi OS

1.  Download Raspberry Pi Imager: https://www.raspberrypi.com/software/

2.  Select: Raspberry Pi OS Lite

3.  In Advanced Options:

    -   Set hostname (example: dronesvr)
    -   Enable SSH
    -   Set username and password
    -   Configure Wi-Fi if required for provisioning

4.  Flash the SD card and boot the device.

5.  Update the system:
```
sudo apt update && sudo apt upgrade -y
```

> [!NOTE]
> Everything beyond this point is for system security hardening. If you do not require enhanced security in your deployment (such as testing and lab environments) you may proceed to the next section [Software and Dependencies Installation](/data_ferry/software_installation/README.md).

## 3. Optional: Enable Full Disk Encryption Using LUKS

If enhanced physical security is required, encrypt the root partition.

**Important:** Backup all data or clone your SD Card before proceeding if your are using an existing installation or system.

### 3.1 Install Required Tools
```
sudo apt install -y cryptsetup cryptsetup-initramfs
```

### 3.2 Identify Root Partition

```
lsblk
```
Typically the root partition is:

/dev/mmcblk0p2

### 3.3 Encrypt the Partition
```
sudo cryptsetup luksFormat /dev/mmcblk0p2
```

Confirm by typing YES and entering a strong passphrase.

### 3.4 Open the Encrypted Partition
```
sudo cryptsetup luksOpen /dev/mmcblk0p2 cryptroot
```

This creates:

/dev/mapper/cryptroot

### 3.5 Retrieve the LUKS UUID
```
sudo blkid /dev/mmcblk0p2
```

Copy the UUID value.

### 3.6 Configure /etc/crypttab

Edit:
```
sudo nano /etc/crypttab
```

Add:
```
cryptroot UUID=`<your-luks-uuid>` none luks
```

Explanation:

-   cryptroot: mapped device name
-   UUID: encrypted partition identifier
-   none: prompt for passphrase at boot
-   luks: specify LUKS format

### 3.7 Update /etc/fstab

Edit:
```
sudo nano /etc/fstab
````

Replace root entry with:
```
/dev/mapper/cryptroot / ext4 defaults,noatime 0 1
```

Ensure filesystem type matches your configuration.


### 3.8 Regenerate initramfs
```
sudo update-initramfs -u -k all
```

This embeds cryptsetup support into early boot.


### 3.9 Test Encryption

Reboot:
```
sudo reboot
```

You should be prompted to unlock cryptroot during boot.

If successful, encryption is configured properly.


## 4. Advanced Unlock Options

Choose ONE unlock method based on operational requirements.

### 4.1 USB Keyfile Auto-Unlock

This allows automatic unlock when a USB device containing the keyfile is
present.

1. Format USB Device
```
lsblk sudo mkfs.ext4 /dev/sda1
````
2. Mount and Create Keyfile
```
sudo mkdir /mnt/usbkey
sudo mount /dev/sda1 /mnt/usbkey
```
```
sudo dd if=/dev/urandom of=/mnt/usbkey/cryptkey.bin bs=4096 count=1
sudo chmod 600 /mnt/usbkey/cryptkey.bin
```

3. Add Key to LUKS
```
sudo cryptsetup luksAddKey /dev/mmcblk0p2 /mnt/usbkey/cryptkey.bin
```
4. Modify /etc/crypttab
```
sudo nano /etc/crypttab
```
add
```
cryptroot UUID=`<your-luks-uuid>` /usbkey/cryptkey.bin luks
```

5. Add USB partition to /etc/fstab:
```
sudo nano /etc/fstab
```
add
```
UUID=`<usb-uuid>` /usbkey ext4 defaults 0 2
```

6. Rebuild initramfs:
```
sudo update-initramfs -u
```

System auto-unlocks when USB is inserted. If missing, system prompts for passphrase.

### 4.2 Hidden Partition Keyfile

Create a small separate partition on the SD card for key storage.

Mount partition temporarily:

```
sudo mount /dev/mmcblk0p3 /mnt/keypart
```

Generate key and add to LUKS:

```
sudo dd if=/dev/urandom of=/mnt/keypart/cryptkey.bin bs=4096 count=1
sudo cryptsetup luksAddKey /dev/mmcblk0p2 /mnt/keypart/cryptkey.bin
```

Update /etc/crypttab accordingly and rebuild initramfs.

> [!WARNING]
> Storing key on same physical medium reduces physical security.

### 4.3 Remote Unlock via SSH in initramfs

Install:
```
sudo apt install dropbear-initramfs
```

Add public key to:
```
/etc/dropbear/initramfs/authorized_keys
```

Rebuild initramfs:
```
sudo update-initramfs -u
```

On boot, SSH into the device and run:
```
cryptroot-unlock
```

This enables remote unlocking for unattended field deployments.

## 5. Secure Base System Configuration

-   Disable SSH password authentication
-   Use key-based login only
-   Enable automatic security updates
-   Consider enabling a firewall

Example:
```
sudo nano /etc/ssh/sshd_config 
```

Add
```
PasswordAuthentication no sudo systemctl
```

restart ssh
```
sudo systemctl restart sshd
```

## 6. Final Validation Checklist

-   System updated
-   SSH secured
-   Encryption configured if required
-   initramfs updated
-   Unlock method tested
-   Network connectivity verified

# Optional Full Disk Copy Backup

This part of the guide explains how to create a full disk image of a Raspberry Pi SD
card as a `.img` file for backup, cloning, or deployment.

This creates a raw, sector-by-sector image of the entire SD card.

## Overview

A full disk image:

-   Captures the entire operating system
-   Preserves partition structure
-   Preserves bootloader and filesystem
-   Allows exact cloning to another SD card

The output file will match the full size of the SD card unless
compressed.

## Linux Method (Recommended)

Using a different Linux installation (not the Raspberry Pi install you just made), do the following.

### 1. Insert the SD Card

Insert the Raspberry Pi SD card using a USB adapter.

### 2. Identify the Device

Run:

``` bash
lsblk
```

or:

``` bash
sudo fdisk -l
```

Look for the SD card device, for example:

    /dev/sdb      64G
    ├─/dev/sdb1
    ├─/dev/sdb2

Use the whole device:

    /dev/sdb

Do NOT use:

    /dev/sdb1

### 3. Unmount Mounted Partitions

``` bash
sudo umount /dev/sdb*
```

### 4. Create the Disk Image

``` bash
sudo dd if=/dev/sdb of=raspberrypi_backup.img bs=4M status=progress conv=fsync
```

***Explanation***

-   `if=` input file (SD card device)
-   `of=` output image file
-   `bs=4M` improves performance
-   `status=progress` shows progress
-   `conv=fsync` ensures data is written safely

This process may take several minutes or hours depending on card size.

### 5. Verify the Image (Optional but Recommended)

``` bash
sudo fdisk -l raspberrypi_backup.img
```

You should see the partition table listed.

## macOS Method

### 1. Insert the SD Card

### 2. Identify the Disk

``` bash
diskutil list
```

Look for something like:

    /dev/disk4 (external)

### 3. Unmount the Disk

``` bash
diskutil unmountDisk /dev/disk4
```

### 4. Create the Image

Use raw disk (`rdisk`) for better performance:

``` bash
sudo dd if=/dev/rdisk4 of=raspberrypi_backup.img bs=4m status=progress
```

## Windows Method (GUI)

### Using Win32 Disk Imager

1.  Install Win32 Disk Imager

2.  Insert SD card

3.  Select the correct drive letter

4.  Choose output file name, for example:

        raspberrypi_backup.img

5.  Click **Read**

This creates a full raw image.

## Compressing the Image (Recommended but optional)

Raw images are full SD card size.

If using a 64 GB card, the image will be 64 GB even if a small percentage of the disk is used.

Compressing or reducing the size of the image allows for expidited cloning, reduced file transfer and stoarge of disk images. 
These operations are imperative for system backups and development environments where iterative code updates and regression testing require a "clean" working image.

### Using PiShrink  (Recommended method)

PiShrink is a open source utility that will resize Raspberry Pi images by removing and reallocating unused image space.
For reference the 64 GB image from the project drone server was reduced to 6 GB with this utility.

For guidance, download and usage of this application please see the author's [GitHub project page.](https://github.com/Drewsif/PiShrink)

### Using gzip

``` bash
gzip raspberrypi_backup.img
```

### Using xz (Better Compression)

``` bash
xz -z -T0 raspberrypi_backup.img
```

### Windows

Use build in .zip utlity or 7zip utility for better compression.

## Restoring an Image

### Linux or macOS

``` bash
sudo dd if=raspberrypi_backup.img of=/dev/sdb bs=4M status=progress conv=fsync
```

Replace `/dev/sdb` with your SD card device.

------------------------------------------------------------------------

### Windows

Use Win32 Disk Imager and click **Write**.

> [!WARNING]
> -   Always verify the correct device before running `dd`
> -   Writing to the wrong disk will erase it
> -   Use the full device (`/dev/sdb`), not a partition (`/dev/sdb1`)
> -   Ensure the SD card is not mounted before imaging

## Optional: Deployment Workflow Tip

For reproducible Raspberry Pi deployments:

-   Configure a master SD card
-   Fully update and test it
-   Create a clean disk image
-   Compress and store versioned backups
-   Use the image for field cloning

Example naming convention:

    drone_server_v1.3_2026-03-02.img.xz

# Next Step

Proceed to:

[Software and Dependencies Installation](/data_ferry/software_installation/README.md)
