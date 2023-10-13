# usbip-server

usbip is a part of the linux kernel (kernel version >= 3.17) and is used to export USB devices over TCP network from one computer (server) to another (client)
The usbip protocol can also be used to create a playground for USB stack and/or USB device development.

The goal for this project is to implement a usbip-server that can handle USB devices in a VSP (Virtual System Prototype) enviromnent.

## Requrements
- linux kernel >= 3.17
- package linux-tools-generic [apt-get install linux-tools-generic]

## Disclaimer

!!! WARNING !!!

Playing around with kernel modules and Virtual USB devices can cause the kernel to hang!

!!! WARNING !!!

## Build
- cd usbip-server
- make

## Start

#### HidMouse
- .x86/HidMouse -vv

#### BulkIO
- .x86/BulkIO -vv

###### Test Tool
- sudo ./tools/test_bulkio.py

## Attach Virtual Device

### Load Module
- sudo modprobe vhci-hcd

### List Devices
- usbip list -r **host**

### Attach Virtual USB Device
- sudo usbip attach -b 1-1 -r **host**

### Detach Virtual USB Device
- sudo usbip detach -p 0

## Current State
- Handles 1 (one) device over the usbip protocol
- USB Stack handles a single interrupt endpoint (at least)
- A HID Mouse Device is implemented
- A BulkIO Device is implemented

## Todo
- Handle several virtual USB devices in the UsbIpServer
- Implement fully functional USB stack
