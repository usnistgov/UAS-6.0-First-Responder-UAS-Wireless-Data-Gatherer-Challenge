# Data Ferry (Drone Server) System Installation Guide

This guide describes how to install and provision Raspberry Pi OS for the Data Ferry platform used in the UAS 6.0 project. It includes optional full-disk encryption guidance using LUKS.

1. [Raspbian OS Installation](/data_ferry/system_install/README.md) <--You are here
2. [Software and Dependencies Installation](/data_ferry/software_installation/README.md)
3. [Network Configuration and Component Servers](/data_ferry/network_configuration/README.md)
4. [PKI_configuration](/data_ferry/PKI_configuration/README.md)
5. [Data Ferry Usage, Server Management, and Debugging](/data_ferry/server_management/README.md)

**Note:** Portions of this section were generated with the assistance of ChatGPT (OpenAI). Authors have reviewed and approved steps outlined in this section.

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

    sudo apt update sudo apt upgrade -y

**NOTE:** Everything beyond this point is for system security hardening. If you do not require enhanced security in your deployment (such as testing and lab environments) you may proceed to the next section [Software and Dependencies Installation](/data_ferry/software_installation/README.md).

## 3. Optional: Enable Full Disk Encryption Using LUKS

If enhanced physical security is required, encrypt the root partition.

Important: Backup all data before proceeding.

### 3.1 Install Required Tools
```
sudo apt install cryptsetup cryptsetup-initramfs
```

### 3.2 Identify Root Partition

lsblk

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

Field explanation:

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

Choose unlock method based on operational requirements.

### 4.1 USB Keyfile Auto-Unlock

This allows automatic unlock when a USB device containing the keyfile is
present.

1. Format USB Device
```
lsblk sudo mkfs.ext4 /dev/sda1
````
2. Mount and Create Keyfile
```
sudo mkdir /mnt/usbkey sudo mount /dev/sda1 /mnt/usbkey
```
```
sudo dd if=/dev/urandom of=/mnt/usbkey/cryptkey.bin bs=4096 count=1 sudo
chmod 600 /mnt/usbkey/cryptkey.bin
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

Note: Storing key on same physical medium reduces physical security.

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

# Next Step

Proceed to:

[Software and Dependencies Installation](/data_ferry/software_installation/README.md)
