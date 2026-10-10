# usbip-server Example Devices

Each page shows how to run one of the example devices, what it prints and how it is
built, as a starting point for your own device.

| Device | USB id | |
|---|---|---|
| [Bulk Device](usb_bulk_device.md) | `fa00:c001` | Vendor class device with one bulk OUT and one bulk IN endpoint |
| [CDC Serial Port](usb_cdc_device.md) | `fa00:0001` | CDC ACM serial port, shows up as `/dev/ttyACMx` |
| [Mouse](usb_mouse_device.md) | `fa00:0002` | HID boot mouse that moves the pointer in a circle |
| [Keyboard](usb_keyboard_device.md) | `fa00:0003` | HID boot keyboard that types a greeting |
| [Audio Input](usb_audio_input_device.md) | `fa00:0004` | USB Audio Class 1 microphone over an isochronous endpoint |
| [GUD Display](usb_gud_display_device.md) | `1d50:614d` | 320x240 display using the GUD protocol, shows up as a monitor |

The server, its library and how to build everything are described in the
[README](https://github.com/freand76/usbip-server#readme).
