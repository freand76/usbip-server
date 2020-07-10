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

const char* stringArray[3] = {
    "Bulk Manufacturer",
    "Bulk Device Name",
    "ABC123",
};

class BulkInputEndpoint : public UsbEndpoint {
public:
    BulkInputEndpoint() : UsbEndpoint(0x81, 2, 64, 255) {};
    int Data(uint8_t* inData, uint8_t* outBuffer, int length);
};

class BulkOutputEndpoint : public UsbEndpoint {
public:
    BulkOutputEndpoint() : UsbEndpoint(0x01, 2, 64, 255) {};
    int Data(uint8_t* inData, uint8_t* outBuffer, int length);
};

class BulkIO : public UsbDevice {
public:
    BulkIO(uint16_t vid,
             uint16_t pid,
             uint16_t bcdVer) :
    UsbDevice(vid, pid, bcdVer, 0, 0, 0, 1, configurationList, &usbString, 1, 2, 3) {
    };
private:
    BulkInputEndpoint  inputEndpoint;
    BulkOutputEndpoint outputEndpoint;
    UsbEndpoint* endpointList[2] = { &inputEndpoint, &outputEndpoint };
    UsbInterface interface = { 0, 0xff, 0, 0, 2, endpointList };
    UsbInterface* interfaceList[1] = { &interface };
    UsbConfiguration config = { 11, 1, interfaceList };
    UsbConfiguration* configurationList[1] = { &config };
    UsbString usbString = { 3, stringArray };
};
