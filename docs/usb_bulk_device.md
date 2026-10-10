# Bulk Device

A vendor specific device with one bulk OUT and one bulk IN endpoint, the simplest way to
move data between a host program and a device. It prints everything it receives and
answers each batch with `Message Counter: <n>`.

| | |
|---|---|
| Program | `out/usb_bulk_device` |
| Source | [src/usb_bulk/](https://github.com/freand76/usbip-server/tree/main/src/usb_bulk) |
| USB id | `fa00:c001` |
| Interface | class `0xff` (vendor specific), no kernel driver binds to it |
| Endpoints | `0x01` bulk OUT, `0x81` bulk IN, 512 byte packets |

## Try It

Build, start the device and attach it, see Attaching and Detaching in the
[README](https://github.com/freand76/usbip-server/blob/main/README.md):

```
$ make
$ out/usb_bulk_device &
$ sudo usbip attach -r localhost -b 1-1
$ lsusb -d fa00:c001
Bus 005 Device 007: ID fa00:c001 Virtual UsbIP Virtual Bulk Device
```

No kernel driver claims the device, so a program talks to it directly through libusb.
[tools/test_bulkio.py](https://github.com/freand76/usbip-server/blob/main/tools/test_bulkio.py) does that with
[uv](https://docs.astral.sh/uv/), which installs its Python dependencies by itself:

```
$ uv run tools/test_bulkio.py
RX: 'Message Counter: 1'
```

The script sends five `Hello World <n>, ` messages, then prints what the device answers
until it has been quiet for two seconds. It needs access to the device node, see
Accessing the Devices Without root in the [README](https://github.com/freand76/usbip-server/blob/main/README.md).

Detach with `sudo usbip detach -p 0`.

## What You See

```
USB BULK Device Activated
USBIPServer: Connected
USBIPProtocol: USBIP_CMD_REQ_DEV_IMPORT '1-1'
Control Data (0)
Input: Hello World 0, Hello World 1, Hello World 2, Hello World 3, Hello World 4,
```

- `Control Data (0)` is the host's `SET_CONFIGURATION`, which every control callback
  receives.
- `Input:` lists what arrived on the OUT endpoint during the last half second.

## How It Works

- `endpoint_data()` is the endpoint data callback. It runs on the server thread and
  appends the data to a 512 byte input buffer, data that does not fit is dropped.
- The main loop checks the input buffer twice a second inside a critical section, so
  the callback can not change it meanwhile. When there was input it prints it and
  answers with `usbip_device_transmit(1, ...)`.
- `usbip_device_transmit()` waits until the host has read the previous answer, so
  without a program reading the IN endpoint the main loop stops after its first answer.
- A host read with nothing to send waits, as with a real device, so the script ends
  its reads with a timeout.

## Prototyping

This is the starting point for any device that a host program drives over libusb: a
measurement device, a firmware updater, a custom protocol. Change the descriptors in
[usb_bulk_descriptors.h](https://github.com/freand76/usbip-server/blob/main/src/usb_bulk/usb_bulk_descriptors.h), replace the echo logic
in the main loop, and handle vendor control requests in `control_data()`.

## Troubleshooting

- `uv run tools/test_bulkio.py` fails with "Access denied": install the udev rule from
  Accessing the Devices Without root in the [README](https://github.com/freand76/usbip-server/blob/main/README.md) and attach again.
- `ValueError: Device not found`: the device is not attached, check `usbip port`.
