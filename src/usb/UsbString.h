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

#ifndef USB_STRING_H
#define USB_STRING_H

#include <stdint.h>

class UsbString {
public:
    UsbString(uint8_t bNumStrings,
              const char** stringArray) {
        this->bNumStrings = bNumStrings;
        this->stringArray = stringArray;
    }
    uint8_t bNumStrings;
    const char** stringArray;
};

#endif // USB_STRING_H
