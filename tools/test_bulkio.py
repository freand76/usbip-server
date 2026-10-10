#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.9"
# dependencies = [
#     "pyusb",
#     "libusb-package",
# ]
# ///

"""Tools to send data to and receive data fram an USB bulk device

Run with "uv run tools/test_bulkio.py", uv installs the dependencies listed above.
"""

import time

import libusb_package
import usb.core
import usb.util

# find our device, using the libusb library that comes with libusb-package
dev = libusb_package.find(idVendor=0xFA00, idProduct=0xC001)

# was it found?
if dev is None:
    raise ValueError("Device not found")

# set the active configuration. With no arguments, the first
# configuration will be the active one
dev.set_configuration()

# get an endpoint instance
cfg = dev.get_active_configuration()
intf = cfg[(0, 0)]

epIn = usb.util.find_descriptor(
    intf,
    # match the first OUT endpoint
    custom_match=lambda e: usb.util.endpoint_direction(e.bEndpointAddress)
    == usb.util.ENDPOINT_IN,
)

epOut = usb.util.find_descriptor(
    intf,
    # match the first OUT endpoint
    custom_match=lambda e: usb.util.endpoint_direction(e.bEndpointAddress)
    == usb.util.ENDPOINT_OUT,
)

assert epIn is not None
assert epOut is not None

# write data to USB device
for idx in range(0, 5):
    test_str = f"Hello World {idx}, "
    epOut.write(test_str)

time.sleep(0.5)

# read data from USB device until it has nothing more to say, a read only returns when
# the device has sent data, so silence shows up as a timeout
READ_TIMEOUT_MS = 2000
while True:
    try:
        response = bytearray(epIn.read(64, timeout=READ_TIMEOUT_MS))
    except usb.core.USBTimeoutError:
        break
    print(f"RX: '{response.decode('utf-8')}'")
