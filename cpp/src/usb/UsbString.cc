/*******************************************************
 usbip-server - a platform for USB device prototyping

 Fredrik Andersson
 Copyright 2016, All Rights Reserved.

 This software may be used by anyone for any reason so
 long as the copyright notice in the source files
 remains intact.

 code repository located at:
        http://github.com/freand76/usbip-server
********************************************************/

#include "UsbString.h"
#include "UsbDevice.h"
#include "UsbUtil.h"

using namespace UsbUtil;

int UsbString::GetStringDescriptor(uint8_t bDescriptorIndex, uint8_t *buffer, int offset) {
    int pos = offset;
    if (bDescriptorIndex == 0) {
        /* Reply with language string */
        pos += SetUint(4, buffer, pos, 1);
        pos += SetUint(3, buffer, pos, 1);
        pos += SetUint(0x0409, buffer, pos, 2);
        return pos;
    } else if (bDescriptorIndex <= bNumStrings) {
        /* Initialize length to 0 */
        pos += SetUint(0, buffer, pos, 1);
        pos += SetUint(3, buffer, pos, 1);
        /* Copy ASCII string to UniCode string */
        uint8_t *stringPtr = (uint8_t *)stringArray[bDescriptorIndex - 1];
        do {
            int c = *stringPtr++;
            pos += SetUint(c, buffer, pos, 2);
        } while (*stringPtr != 0);

        /* Set Length */
        SetUint(pos, buffer, 0, 1);
        return pos;
    }

    return EP_STALL;
}
