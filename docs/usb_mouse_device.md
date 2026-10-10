# Mouse

A HID boot mouse. Linux binds its `usbhid` driver to it, and it moves your mouse pointer
in a circle, one lap per second, for as long as it is attached.

| | |
|---|---|
| Program | `out/usb_mouse_device` |
| Source | [src/usb_mouse/](https://github.com/freand76/usbip-server/tree/main/src/usb_mouse) |
| USB id | `fa00:0002` |
| Interface | HID (class `0x03`), boot interface subclass, mouse protocol |
| Endpoint | `0x81` interrupt IN, 4 byte reports, polled every 16 ms |

*The pointer keeps circling while the mouse is attached, keep a terminal with the
detach command ready.*

## Try It

```
$ make
$ out/usb_mouse_device &
$ sudo usbip attach -r localhost -b 1-1
```

The pointer starts circling. Stop it with `sudo usbip detach -p 0`, or stop the device
with Ctrl-C three times.

## What You See

```
USB Mouse Device Activated
USBIPServer: Connected
USBIPProtocol: USBIP_CMD_REQ_DEV_IMPORT '1-1'
```

The kernel log (`journalctl -k`) shows the new input device:

```
input: Virtual UsbIP Virtual HID Mouse as /devices/platform/vhci_hcd.0/usb5/5-1/5-1:1.0/...
hid-generic 0003:FA00:0002.000F: input,hidraw8: USB HID v1.11 Mouse [Virtual UsbIP Virtual HID Mouse] on usb-vhci_hcd.0-1/input0
```

## How It Works

- Each report is relative movement: a button byte, then signed X and Y steps.
- The main loop computes points on a circle with a radius of 200 and sends the
  difference to the previous point with `usbip_device_transmit()`.
- It never sleeps itself. The server delivers at most one report per 16 ms polling
  interval and `usbip_device_transmit()` waits until the host has fetched the previous
  report, so the loop runs at exactly the polling rate: 62 steps of 16 ms make one
  lap per second.
- `control_data()` answers the HID `GET_DESCRIPTOR` request for the report descriptor.

## Prototyping

Start here for pointing devices: mice, trackballs, joysticks, touch pads, or anything
that reports movement. Change the polling interval in
[usb_mouse_descriptors.h](https://github.com/freand76/usbip-server/blob/main/src/usb_mouse/usb_mouse_descriptors.h) and the report rate
follows, as it would on real hardware.

## Troubleshooting

- The pointer keeps circling after one Ctrl-C: the server stops after three, see the
  message it prints. Detaching stops the movement at once.
