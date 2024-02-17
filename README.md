# USB IP Server for USB Device Prototyping

## usbip-server

usbip is a part of the linux kernel (kernel version >= 3.17) and it is used to export
USB devices over TCP network from one computer (server) to another (client)

The usbip protocol can also be used to, like in this project, create a playground for
USB stack and/or USB device development.

The goal for this project is to implement a usbip-server that can be used for USB
device prototyping on a linux host.

The code in **src/** is a C implementation of a C++ version I did back in 2016.
The C++ version is still available in the **cpp/** folderc, but (I do not think) it will not be updated.

## Requirements

- linux kernel >= 3.17
- package linux-tools-generic [apt-get install linux-tools-generic]

## Disclaimer

*!!! WARNING !!! Playing around with kernel modules and Virtual USB devices can cause the kernel to hang!*

## Build C version

```
$ make
```

## Start

### CDC Device (Will be available as a ttyACMx device under linux)

```
$ out/usb_cdc_device
```

### Mouse Device (Will start to move your mouse pointer in a circle)

```
$ out/usb_mouse_device
```

## Attach Virtual Device

### Load Kernel Module to Handle USBIP

```
$ sudo modprobe vhci-hcd
```

### List Devices

```
$ usbip list -r <host>
```

### Attach Virtual USB Device

```
$ sudo usbip attach -b 1-1 -r <host>
```

*Example: If you want to attach the USB device on the machine running the usbip server.*

```
$ sudo usbip attach -b 1-1 -r localhost
```


### Detach Virtual USB Device

```
$ sudo usbip detach -p 0
```

## Current State

- Handles 1 (one) device over the usbip protocol

## Star History

[![Star History Chart](https://api.star-history.com/svg?repos=freand76/usbip-server&type=Date)](https://star-history.com/#freand76/usbip-server&Date)

## Todo

- Implement more device examples
- Understand and handle usbip unlink command better

