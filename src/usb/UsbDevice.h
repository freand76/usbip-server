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

#ifndef USB_DEVICE_H
#define USB_DEVICE_H

#include <stdint.h>
#include "UsbConfiguration.h"

#define EP_STALL (-32)

class UsbDevice {
public:
    UsbDevice(uint16_t idVendor,
              uint16_t idProduct,
              uint16_t bcdDevice,
              uint8_t bDeviceClass,
              uint8_t bDeviceSubClass,
              uint8_t bDeviceProtocol,
              uint8_t bNumConfigurations,
              UsbConfiguration** configurationArray);

    int TxRx(uint8_t endpoint, uint8_t* setup, uint8_t* data, uint8_t* replyBuffer, int bufLength);
    int DeviceRequest(uint8_t* setup, uint8_t* data, uint8_t* replyBuffer, int bufLength);
    int InterfaceRequest(uint8_t* setup, uint8_t* data, uint8_t* replyBuffer, int bufLength);
    int OutRequest(uint8_t* setup, uint8_t* data, uint8_t* replyBuffer, int bufLength);
    int GetDescriptor(uint8_t* setup, uint8_t* data, uint8_t* replyBuffer, int bufLength);

    int InRequest(uint8_t* setup, uint8_t* data, uint8_t* replyBuffer, int bufLength);
    void Disconnect();
    bool IsConnected() { return deviceConnected; };

    bool deviceConnected;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bConfigurationValue;
    uint8_t bNumConfigurations;
    UsbConfiguration** configurationArray;
};

#endif // USB_DEVICE_H
