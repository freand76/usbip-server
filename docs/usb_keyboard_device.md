# Keyboard

A HID boot keyboard. Linux binds its `usbhid` driver to it, and it types into your
desktop like any keyboard: one second after the host has configured it, it types
`Who is typing...`.

| | |
|---|---|
| Program | `out/usb_keyboard_device` |
| Source | [src/usb_keyboard/](../src/usb_keyboard/) |
| USB id | `fa00:0003` |
| Interface | HID (class `0x03`), boot interface subclass, keyboard protocol |
| Endpoint | `0x81` interrupt IN, 8 byte reports, polled every 8 ms |

*The text goes to whichever window has focus, a terminal included.*

## Try It

```
$ make
$ out/usb_keyboard_device &
$ sudo usbip attach -r localhost -b 1-1
```

Click into an empty editor window before the attach completes. A second later
`Who is typing...` appears. The keyboard types once per attach; detach with
`sudo usbip detach -p 0` and attach again to type again.

## What You See

```
USB Keyboard Device Activated
USBIPServer: Connected
USBIPProtocol: USBIP_CMD_REQ_DEV_IMPORT '1-1'
Configuration Active
```

`Configuration Active` is printed when the host sends `SET_CONFIGURATION`, the moment
the keyboard is ready to be used.

## How It Works

- `control_data()` answers the HID `GET_DESCRIPTOR` request for the report descriptor,
  and sets `configuration_active` on `SET_CONFIGURATION` with a configuration other
  than 0.
- The main loop waits for `configuration_active`, then waits one more second: the
  desktop starts listening to a new keyboard a moment after it is configured, and keys
  typed before that are lost.
- Each character becomes a key down report, sent with `usbip_device_transmit()`,
  followed 20 ms later by an all zero key up report. Characters are 100 ms apart.
- `ascii_to_hid()` turns characters into HID usage codes plus the shift modifier.
  Letters, digits, space, Enter, Backspace, Tab and the punctuation of a US keyboard
  are supported.
- The server delivers at most one report per 8 ms polling interval, so the main loop
  can simply call `usbip_device_transmit()` and runs at the speed the descriptors
  promise.

## Prototyping

Start here for keyboards, keypads, barcode scanners, foot pedals, or anything else that
sends key presses. The boot protocol report (modifier byte, reserved byte, six key
codes) works on every host, the BIOS included.

## Troubleshooting

- Wrong characters: the keyboard sends key positions, not characters, and the host's
  keyboard layout decides what they mean. `ascii_to_hid()` assumes a US layout, so
  with another layout punctuation can come out differently.
- Nothing is typed: check that the keyboard printed `Configuration Active`, and that a
  window that accepts text has focus.
