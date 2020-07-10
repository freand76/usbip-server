#!/usr/bin/python3

import usb.core
import usb.util

# find our device
dev = usb.core.find(idVendor=0x00fa, idProduct=0xc001)

# was it found?
if dev is None:
    raise ValueError('Device not found')

# set the active configuration. With no arguments, the first
# configuration will be the active one
dev.set_configuration()

# get an endpoint instance
cfg = dev.get_active_configuration()
intf = cfg[(0,0)]

epIn = usb.util.find_descriptor(
    intf,
    # match the first OUT endpoint
    custom_match = \
    lambda e: \
        usb.util.endpoint_direction(e.bEndpointAddress) == \
        usb.util.ENDPOINT_IN)

epOut = usb.util.find_descriptor(
    intf,
    # match the first OUT endpoint
    custom_match = \
    lambda e: \
        usb.util.endpoint_direction(e.bEndpointAddress) == \
        usb.util.ENDPOINT_OUT)

assert epIn is not None
assert epOut is not None

# write data to USB device
for idx in range(0, 5):
    str = "Hello World (%d)"%idx
    epOut.write(str)

# read data from USB device
a = epIn.read(64)
print("Received Data:", a.tostring().decode('utf-8'))

