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

typedef struct  {
    bool free;
    int but;
    int x;
    int y;
} MouseEvent_t;


class HidMouseEndpoint : public UsbEndpoint {
public:
    HidMouseEndpoint() : UsbEndpoint(0x81, 3, 64, 255) {};
    int Data(uint8_t* inData, uint8_t* outBuffer, int length);
    MouseEvent_t event = { true, 0, 0, 0 };
};

class HidMouseInterface : public UsbInterface {
public:
    HidMouseInterface(uint8_t bInterfaceNumber,
                      uint8_t bNumEndpoints,
                      UsbEndpoint** endpointArray) :
    UsbInterface(bInterfaceNumber, 0, 3, 1, 2, 0, bNumEndpoints, endpointArray) {
    };
    int GenerateConfigurationDescriptor(uint8_t* buffer, int offset);
    int GetDescriptor(uint8_t* setup, uint8_t* data, uint8_t* replyBuffer, int bufLength);

private:
    int GenerateHIDReportDescriptor(uint8_t* buffer, int offset);
};

class HidMouse : public UsbDevice {
public:
    HidMouse(uint16_t vid,
             uint16_t pid,
             uint16_t bcdVer) :
    UsbDevice(vid, pid, bcdVer, 0, 0, 0, 1, configurationList) {
    };

    bool Move(int but, int x, int y);
private:
    HidMouseEndpoint endpoint;
    UsbEndpoint* endpointList[1] = { &endpoint };
    HidMouseInterface interface = { 0, 1, endpointList };
    UsbInterface* interfaceList[1] = { &interface };
    UsbConfiguration config = { 0x77, 0, 0xc0, 100, 1, interfaceList };
    UsbConfiguration* configurationList[1] = { &config };
};
