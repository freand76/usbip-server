# CDC Serial Port

A CDC ACM (Abstract Control Model) device, the USB class of virtual serial ports. Linux
binds its `cdc_acm` driver to it and the device shows up as `/dev/ttyACMx`.

| | |
|---|---|
| Program | `out/usb_cdc_device` |
| Source | [src/usb_cdc/](../src/usb_cdc/) |
| USB id | `fa00:0001` |
| Interfaces | communication interface (class `0x02`, ACM), data interface (class `0x0a`) |
| Endpoints | `0x82` interrupt IN for notifications, `0x01` bulk OUT and `0x81` bulk IN for data, 512 byte packets |

## Try It

```
$ make
$ out/usb_cdc_device &
$ sudo usbip attach -r localhost -b 1-1
$ ls /dev/ttyACM*
/dev/ttyACM0
```

Open the port with any terminal program, for example `picocom` (quit with Ctrl-A
Ctrl-X):

```
$ picocom -b 115200 /dev/ttyACM0
Tick 0
Tick 1
...
```

What you type is printed by the device. Writing from a script works too:

```
$ echo "hi!" > /dev/ttyACM0
```

The port belongs to the `dialout` group, add yourself to it to open it without root.

Detach with `sudo usbip detach -p 0`.

## What You See

```
USB CDC Device Activated
USBIPServer: Connected
USBIPProtocol: USBIP_CMD_REQ_DEV_IMPORT '1-1'
USB Reset
Linecoding: Baud=115200 CharFormat=0 Parity=0 DataBits=8
Linestate: DTR: HIGH RTS: HIGH
Input: 0x68, 0x69, 0x21, 0x0a,
Linestate: DTR: LOW  RTS: LOW
```

- `Linecoding` is the host's `SET_LINE_CODING` request with the terminal's settings.
  There is no real serial line, so they are only printed.
- `Linestate` is `SET_CONTROL_LINE_STATE`. A terminal program raises DTR when it opens
  the port and drops it when it closes the port.
- `Input` shows the received bytes in hex.
- `USB Reset` is printed when the host attaches or detaches the device.

## How It Works

- `control_data()` handles the CDC class requests `SET_LINE_CODING`,
  `SET_CONTROL_LINE_STATE` and `SEND_BREAK`.
- The main loop sends `Tick <n>` twice a second, but only while DTR is high, the way a
  device waits for a terminal before it talks.
- `endpoint_data()` collects received bytes in a 10 byte buffer that the main loop
  prints twice a second, bytes beyond 10 in half a second are dropped.
- `reset()` drops DTR when the host attaches or detaches, so the ticks stop with the
  host.

## Prototyping

Start here for any device that should look like a serial port: a debug console, a
modem, a GPS receiver or a microcontroller bridge. The host side needs no special
software, every terminal program and serial library works with it.

## Troubleshooting

- No `Tick` lines: DTR is low. Closing the terminal drops it, and some programs never
  raise it. With `stty` settings that clear `HUPCL`, closing the port does not drop DTR.
- `Permission denied` on `/dev/ttyACM0`: you are not in the `dialout` group.
