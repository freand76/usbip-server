# usbip-server

usbip is a part of the linux kernel (kernel version >= 3.17) and is used to export USB devices over TCP network from one computer (server) to another (client)
The usbip protocol can also be used to create a playground for USB stack and/or USB device development.

The goal for this project is to implement a usbip-server that can handle USB devices in a VSP (Virtual System Prototype) enviromnent.

## Requrements
- linux kernel > 3.17
- package linux-tools-generic [apt-get install linux-tools-generic]
- package libwxgtk3.0-dev     [apt-get install libwxgtk3.0-dev]

## Build
- cd usbip-server
- make
- .x86/TestApp

## Attach Virtual Device

### Load Module
sudo modprobe vhci-hcd

### List Devices
usbip list -r <host>

### Attach Virtual USB Device
sudo usbip attach -b 1-1 -r <host>

## Todo
- Handle several virtual USB devices in the UsbIpServer
- Implement fully functional USB stack
- Remove dependency to wxWidgets

