# USB IP Server for USB Device Prototyping

**Build USB devices without building hardware.**

Write the device side of a USB device in plain C, run it as an ordinary Linux program,
and attach it to your computer as if a real device had been plugged in. The kernel
enumerates it, loads its real drivers and hands it to your applications: a HID device
becomes a mouse or keyboard that moves your pointer and types into your editor, a CDC
device becomes a serial port that any terminal program can open.

```
$ out/usb_cdc_device &
$ sudo usbip attach -r localhost -b 1-1
$ journalctl -k
usb 5-1: new high-speed USB device number 18 using vhci_hcd
usb 5-1: New USB device found, idVendor=fa00, idProduct=0001, bcdDevice= 2.00
usb 5-1: Product: Virtual CDC Device
usb 5-1: Manufacturer: Virtual UsbIP
cdc_acm 5-1:1.0: ttyACM0: USB ACM device
```

### Why

Developing a USB device usually means a microcontroller board, a programmer, and a
flash-and-replug cycle for every change. Here the device is a normal process:

- **Fast turnaround**: edit, `make`, run. A new descriptor or class request is seconds
  away from being tested by the real Linux USB stack.
- **Your usual tools**: `printf`, `gdb`, AddressSanitizer and Valgrind all work on
  device code, because it is just a program.
- **Real host behaviour**: the device is enumerated and driven by the same kernel
  drivers that would handle the finished hardware, so most mistakes in descriptors and
  requests show up just as they would with a real device.
- **No hardware at all**: nothing to buy, wire or flash.

### What It Is Good For

- Prototyping the USB interface of a device before the hardware exists, then moving the
  descriptors and request handling to the firmware.
- Developing host software, such as a libusb application or a tool that talks to a
  serial port, against a device that is always available and behaves exactly as you
  program it, including on purpose doing the wrong thing.
- Learning USB: attach a device and watch enumeration, descriptors and transfers happen
  in the kernel log, `lsusb -v` or Wireshark.

Six ready-to-run example devices are included: a bulk device, a CDC serial port, a
mouse, a keyboard, a microphone and a display.

## How It Works

[USB/IP](https://docs.kernel.org/usb/usbip_protocol.html) is part of the Linux kernel. It
carries USB traffic over TCP, normally so that a USB device plugged into one computer can
be used from another. This project implements the *device* end of that protocol in
user space, so there is no physical device at all:

```
  your device code                                    Linux host
 ┌──────────────────────┐   USB/IP over TCP   ┌──────────────────────────┐
 │ descriptors          │      port 3240      │ usbip attach             │
 │ callbacks            │◀───────────────────▶│ vhci_hcd (virtual host   │
 │ usbip_device_transmit│                     │   controller)            │
 ├──────────────────────┤                     │ USB core + real drivers  │
 │ libusbip_server      │                     │ (usbhid, cdc_acm, usbfs) │
 └──────────────────────┘                     └──────────────────────────┘
```

The library answers the standard USB requests (descriptors, status, configuration),
queues transfers until your device has data, like a real device answering with NAK,
and runs interrupt and isochronous endpoints at the interval from your descriptors, so
streams such as audio play in real time. Your code only handles what is specific to the
device.

## Requirements

- Linux with the `vhci-hcd` kernel module (kernel 3.17 or later)
- The `usbip` command line tool
- `gcc` and `make`

### Installing on Ubuntu

On Ubuntu, `/usr/bin/usbip` from `linux-tools-common` is only a wrapper: it runs the
`usbip` built for the running kernel, so the tools package must match `uname -r`. There
is no separate `usbip` package.

Ubuntu 22.04 and 24.04 ship `vhci-hcd` in `linux-modules-extra`. Desktop and server
installs already have it, minimal and cloud images may not:

```
$ sudo apt install build-essential linux-tools-common linux-tools-$(uname -r) \
      linux-modules-extra-$(uname -r)
```

Ubuntu 26.04 ships `vhci-hcd` with the kernel itself:

```
$ sudo apt install build-essential linux-tools-common linux-tools-$(uname -r)
```

To keep `usbip` working after kernel updates, also install the metapackage that follows
your kernel flavour, for example `linux-tools-generic` for the standard kernel.

### Installing on Debian

```
$ sudo apt install build-essential usbip
```

## Disclaimer

*!!! WARNING !!! Playing around with kernel modules and Virtual USB devices can cause the kernel to hang!*

## Quick Start

```
$ make
$ sudo modprobe vhci-hcd
$ out/usb_cdc_device &
$ sudo usbip attach -r localhost -b 1-1
$ picocom /dev/ttyACM0          # prints "Tick 0", "Tick 1", ... until you quit
$ sudo usbip detach -p 0
```

## Building

```
$ make            # builds all examples into out/
$ V=1 make        # show the full compiler command lines
$ make clean      # remove out/
$ make format     # format the sources with clang-format
$ make gud_display   # the GUD display example, needs SDL3 (libsdl3-dev)
$ make check-python  # check formatting and lint of tools/ with ruff (needs uv)
```

## Example Devices

Each example has its own page with detailed instructions: how to try it, what it prints,
how its code works and what to change for your own device.

| Program | ID | What it does |
|---|---|---|
| [`out/usb_bulk_device`](docs/usb_bulk_device.md) | `fa00:c001` | Vendor class device with one bulk OUT and one bulk IN endpoint. Prints everything it receives and answers with `Message Counter: <n>`. |
| [`out/usb_cdc_device`](docs/usb_cdc_device.md) | `fa00:0001` | CDC ACM serial port, shows up as `/dev/ttyACMx`. Sends `Tick <n>` twice a second while a terminal has the port open, and prints what you type. |
| [`out/usb_mouse_device`](docs/usb_mouse_device.md) | `fa00:0002` | HID boot mouse that moves the pointer in a circle, one lap per second. |
| [`out/usb_keyboard_device`](docs/usb_keyboard_device.md) | `fa00:0003` | HID boot keyboard that types `Who is typing...` one second after the host has configured it. |
| [`out/usb_audio_input_device`](docs/usb_audio_input_device.md) | `fa00:0004` | USB Audio Class 1 microphone, 48 kHz 16 bit mono over an isochronous endpoint. While something records from it, it plays an alien pulse once a second. |
| [`out/usb_gud_display_device`](docs/usb_gud_display_device.md) | `1d50:614d` | 320x240 RGB565 display using the GUD (Generic USB Display) protocol, shows up as a monitor. A window shows what the host draws on it. |

*The mouse moves your real pointer and the keyboard types into whichever window has
focus.*

*The display is built separately with `make gud_display` and needs SDL3. On GNOME,
read its page before attaching it a second time in a session.*

## Attaching and Detaching

The `vhci-hcd` module provides the virtual host controller. Load it once per boot:

```
$ sudo modprobe vhci-hcd
```

List the devices a server offers. The server always offers exactly one device, with bus
id `1-1`:

```
$ usbip list -r localhost
```

Attach it. The kernel log (`journalctl -k -f` or `sudo dmesg -w`) shows the enumeration:

```
$ sudo usbip attach -r localhost -b 1-1
```

Show the attached devices and their port numbers, then detach with the port number:

```
$ usbip port
$ sudo usbip detach -p 0
```

### Running usbip Without sudo

`usbip attach` and `detach` write to files that only root can write. To let a group of
users attach devices, create a `usbip` group, add yourself to it, and install these
three files:

`/etc/udev/rules.d/90-usbip.rules`

```
ACTION=="add", SUBSYSTEM=="platform", KERNEL=="vhci_hcd.*", \
  RUN+="/bin/chgrp usbip /sys%p/attach /sys%p/detach", \
  RUN+="/bin/chmod g+w /sys%p/attach /sys%p/detach"
```

`/etc/modules-load.d/usbip.conf` (loads `vhci-hcd` at boot)

```
vhci-hcd
```

`/etc/tmpfiles.d/usbip.conf` (where `usbip` keeps track of attached ports)

```
d /run/vhci_hcd 0775 root usbip -
```

Reboot, or log in again and reload the module, for the changes to take effect.

### <a name="accessing-the-devices-without-root"></a>Accessing the Devices Without root

Programs that talk to a USB device directly, like `tools/test_bulkio.py` through
`pyusb` or any other libusb application, open its node under `/dev/bus/usb`. By default
only root may write to it. All example devices except the display use vendor id `fa00`,
so one udev rule gives access to all of them (the display is used by a kernel driver and
needs no rule):

`/etc/udev/rules.d/70-usbip-server.rules`

```
SUBSYSTEM=="usb", ATTR{idVendor}=="fa00", MODE="0660", GROUP="plugdev", TAG+="uaccess"
```

- `GROUP="plugdev"` and `MODE="0660"` give read and write access to members of the
  `plugdev` group
- `TAG+="uaccess"` also gives access to the user logged in at the desktop. It is handled
  by `73-seat-late.rules`, so the rule file must sort before it, hence the `70-` prefix
- to match a single device, add its product id: `ATTR{idProduct}=="c001"`

Load the rule and attach the device again, rules are only applied when a device appears:

```
$ sudo udevadm control --reload-rules
$ sudo usbip detach -p 0
$ sudo usbip attach -r localhost -b 1-1
```

Check which attributes a rule can match with `udevadm info -a -n /dev/bus/usb/<bus>/<dev>`,
the bus and device numbers are shown by `lsusb`.

The CDC serial port is a different node, `/dev/ttyACM0`, which belongs to the `dialout`
group. Add yourself to that group to open it without root.

*Vendor id `fa00` is not assigned to this project, it is only meant for devices on your
own machine. A product needs a vendor id of its own, from the USB Implementers Forum.*

### Attaching From Another Machine

The server only accepts connections from the local machine. Set `USBIP_BIND_ADDRESS` to
an IPv4 address to listen on, or `0.0.0.0` for all interfaces:

```
$ USBIP_BIND_ADDRESS=0.0.0.0 out/usb_cdc_device
```

and attach from the other machine with `-r <server address>`.

*Anyone who can reach port 3240 can then attach the device, there is no authentication.*

## Writing Your Own Device

The examples in [src/](src/) are the best starting point: copy the one closest to what
you want to build. The public API is in [include/usbip_server_api.h](include/usbip_server_api.h).

### 1. Descriptors

Write the device, configuration, language id and string descriptors as byte arrays, see
any `*_descriptors.h` file. The device is always reported as **high speed**, so the
descriptors must follow the high speed rules:

- `bMaxPacketSize0` is 64
- bulk endpoints use 512 byte packets
- interrupt and isochronous `bInterval` is an exponent: the polling period is
  2^(bInterval-1) × 125 µs, so 8 means 16 ms and 4 means 1 ms

### 2. The Device

```c
#include "usbip_server_api.h"
#include "my_descriptors.h"

static void control_data(const uint8_t *setup, const uint8_t *data, size_t data_length) {
    /* Class, vendor and interface requests, plus SET_CONFIGURATION */
}

static void endpoint_data(uint8_t ep, const uint8_t *data, size_t data_length) {
    /* Data the host sent to an OUT endpoint */
}

static void reset(void) {
    /* A host attached or went away, forget per-session state */
}

static size_t iso_in_data(uint8_t ep, uint8_t *buffer, size_t max_length) {
    /* Fill one service interval of an isochronous IN endpoint, return the length */
    return 0;
}

static const usbip_device_t my_device = {
    .device_desc = device_descriptor,
    .config_desc = config_descriptor,
    .langid_desc = langid_descriptor,
    .usb_strings = usb_strings,
    .control_data_callback = control_data,
    .ep_data_callback = endpoint_data, /* Optional */
    .reset_callback = reset,           /* Optional */
    .iso_in_callback = iso_in_data,    /* Optional */
};

int main(void) {
    if (!usbip_server_start(&my_device)) {
        return 1;
    }
    while (!usbip_server_interrupted()) {
        /* Produce data for IN endpoints */
        usbip_device_transmit(1, data, length);
    }
    usbip_server_stop();
    return 0;
}
```

### 3. How the Library Behaves

- **Standard requests** are answered for you: `GET_DESCRIPTOR` (device, configuration,
  string, device qualifier, other speed configuration), `GET_STATUS`,
  `GET_CONFIGURATION`, `SET_CONFIGURATION` and endpoint `CLEAR_FEATURE`/`SET_FEATURE`.
  Anything else is stalled.
- **The control callback** receives class, vendor and standard interface requests (for
  example HID `GET_DESCRIPTOR` of the report descriptor). It also receives
  `SET_CONFIGURATION` after the library has handled it, which is the moment the device
  is ready to be used. Answer an IN request with `usbip_device_transmit(0, ...)`,
  `usbip_device_transmit(0, NULL, 0)` is a valid empty answer. An IN request that gets
  no answer is stalled, the way a real device rejects a request it does not support.
- **`usbip_device_transmit()`** hands data for an IN endpoint to the host, at most 512
  bytes at a time. It waits until the host has fetched earlier data on that endpoint,
  just as firmware waits for an endpoint buffer to become free. Data that does not fit
  the host's transfer is delivered in the following transfers. Returns false if the
  data was dropped.
- **Interrupt IN endpoints are paced**: the host receives at most one transfer per
  polling interval, as on a real bus, so a device loop can simply call
  `usbip_device_transmit()` and will run at the speed the descriptors promise.
- **Isochronous endpoints stream in real time**: a transfer of N packets completes N
  service intervals after the previous one on the endpoint, as on a real bus. For an IN
  endpoint the iso IN callback is asked for each packet's data when the transfer
  completes, so it should return exactly one interval's worth of data, for example 48
  audio samples per 1 ms. A packet of an OUT endpoint reaches the endpoint data callback
  as soon as the host sends it.
- **Callbacks run on the server thread.** `usbip_device_transmit()` can not wait there
  (or inside a critical section), so it drops the data and returns false if the
  endpoint still has unread data.
- **Critical sections**: no callback runs between `usbip_device_critical_section_enter()`
  and `usbip_device_critical_section_exit()`. Use them to share data with callbacks,
  and keep them short, the host is waiting.
- **Reset**: when a host attaches or disconnects, unread endpoint data is discarded,
  the configuration goes back to 0 and the reset callback is called, like a USB bus
  reset on real hardware.
- **Ctrl-C** three times stops the server, `usbip_server_interrupted()` tells your main
  loop. Five more times forces an exit if something hangs.

### 4. Build It

Add a section for your device to the [makefile](makefile), following the pattern of
the existing examples, and add the program to `TARGETS`.

## Debugging Tips

- The server prints each connection, import and lost connection on standard output.
- The kernel log shows enumeration and driver errors: `journalctl -k -f`.
- `lsusb -v -d <vid>:<pid>` shows the descriptors as the host parsed them, plus the
  device status and device qualifier.
- [usbmon](https://docs.kernel.org/usb/usbmon.html) together with Wireshark captures
  every request on the virtual bus.

## Limitations

- One device, served to one client at a time, on TCP port 3240 with bus id `1-1`.
- The device is always reported as high speed.
- At most 128 packets per isochronous transfer, enough for Linux drivers and usbfs.
- Transfers larger than 64 KiB are rejected, and one call to
  `usbip_device_transmit()` carries at most 512 bytes.
- No authentication. The server listens on the local machine only, unless
  `USBIP_BIND_ADDRESS` says otherwise.

## Specifications and Further Reading

USB/IP:

- [USB/IP protocol](https://docs.kernel.org/usb/usbip_protocol.html), the protocol this
  server implements
- [Linux USB/IP driver source](https://github.com/torvalds/linux/tree/master/drivers/usb/usbip),
  including `vhci_hcd`, the client side used by `usbip attach`
- [usbip user space tools source](https://github.com/torvalds/linux/tree/master/tools/usb/usbip)

USB specifications from the USB Implementers Forum:

- [USB 2.0 Specification](https://www.usb.org/document-library/usb-20-specification),
  chapter 9 describes descriptors and standard requests
- [Defined Class Codes](https://www.usb.org/defined-class-codes), the class, subclass and
  protocol values for descriptors
- [HID specifications](https://www.usb.org/hid), the HID class definition and the HID
  usage tables used by the mouse and keyboard examples
- [Class Definitions for Communication Devices 1.2](https://www.usb.org/document-library/class-definitions-communication-devices-12),
  the CDC specification behind the serial port example

Introductions to USB:

- [USB in a NutShell](https://www.beyondlogic.org/usbnutshell/usb1.shtml), a readable
  walk through USB transfers, descriptors and enumeration, the best place to start
- [Linux USB API](https://docs.kernel.org/driver-api/usb/index.html), how the Linux host
  side sees USB devices

## History

The code in **src/** is a C implementation of a C++ version I did back in 2016.

The C++ version is still available in the **cpp_version** branch in the **cpp/** folder,
but it will probably not be updated...

## Star History

[![Star History Chart](https://api.star-history.com/svg?repos=freand76/usbip-server&type=Date)](https://star-history.com/#freand76/usbip-server&Date)

## Todo

- Implement more device examples
