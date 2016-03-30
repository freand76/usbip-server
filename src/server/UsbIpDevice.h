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

#ifndef USBIP_DEVICE_H
#define USBIP_DEVICE_H

#include <string>
#include <linux/usb/ch9.h>

#include "UsbDevice.h"

using namespace std;

class UsbIpDevice {
public:
    UsbIpDevice(UsbDevice* d = NULL,
                string path = "",
                string busId = "",
                int busNum = 0,
                int devNum = 0,
                enum usb_device_speed speed = USB_SPEED_HIGH);

    int FillDeviceData(unsigned char* buffer,
                       int offset,
                       bool withInterfaces);

    int TxRx(unsigned char* setupBuffer,
             unsigned char* dataBuffer,
             unsigned char* outBuffer,
             int outBufferLength);

private:
    int CopyString(char* str, int length, unsigned char* buffer, int offset);

    UsbDevice* d;
    int busNum;
    int devNum;
    int speed;
    char pathStrBuf[256];
    char busIdStrBuf[32];
};

#endif // USBIP_DEVICE_H
