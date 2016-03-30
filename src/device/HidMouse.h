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

#include "UsbDevice.h"
#include "UsbConfiguration.h"
#include "UsbInterface.h"
#include "UsbEndpoint.h"

class HidMouse : public UsbDevice {
public:
    HidMouse(uint16_t vid, uint16_t pid, uint16_t bcdVer) : UsbDevice(vid, pid, bcdVer, 3, 0, 0, 1, configurationList) {
    };

    UsbEndpoint endpoint = { 0x81, 3, 64, 10 };
    UsbEndpoint* endpointList[1] = { &endpoint };
    UsbInterface interface = { 0, 0, 3, 0, 0, 0, 1, endpointList };
    UsbInterface* interfaceList[1] = { &interface };
    UsbConfiguration config = { 1, 0, 0xc0, 100, 1, interfaceList };
    UsbConfiguration* configurationList[1] = { &config };
};
