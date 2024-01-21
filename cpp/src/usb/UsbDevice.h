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
#include <string.h>
#include "UsbConfiguration.h"
#include "UsbString.h"

#define EP_STALL (-32)

class UsbDevice {
    public:
        UsbDevice(uint16_t idVendor, uint16_t idProduct, uint16_t bcdDevice, uint8_t bDeviceClass, uint8_t bDeviceSubClass, uint8_t bDeviceProtocol,
                  uint8_t bNumConfigurations, UsbConfiguration **configurationArray, UsbString *usbString = NULL, uint8_t iManufacturer = 0,
                  uint8_t iProduct = 0, uint8_t iSerialNumber = 0);

        int TxRx(uint8_t endpoint, uint8_t *usbSetup, uint8_t *dataIn, uint8_t *dataOut, int transferLength);
        int DeviceRequest(uint8_t *usbSetup, uint8_t *dataIn, uint8_t *dataOut, int transferLength);
        int InterfaceRequest(uint8_t *usbSetup, uint8_t *dataIn, uint8_t *dataOut, int transferLength);

        int OutRequest(uint8_t *usbSetup, uint8_t *dataIn, uint8_t *dataOut, int transferLength);
        int InRequest(uint8_t *usbSetup, uint8_t *dataIn, uint8_t *dataOut, int transferLength);

        int GetDescriptor(uint8_t *usbSetup, uint8_t *dataIn, uint8_t *dataOut, int transferLength);
        int GetDeviceDescriptor(uint8_t *buffer, int transferLength);
        int GetDeviceQualifier(uint8_t *buffer);

        void Disconnect();
        bool IsConnected() {
            return deviceConnected;
        };

        bool deviceConnected;
        uint16_t idVendor;
        uint16_t idProduct;
        uint16_t bcdDevice;
        uint8_t bDeviceClass;
        uint8_t bDeviceSubClass;
        uint8_t bDeviceProtocol;
        uint8_t bConfigurationValue;
        uint8_t bNumConfigurations;
        UsbConfiguration **configurationArray;
        UsbString *usbString;
        uint8_t iManufacturer;
        uint8_t iProduct;
        uint8_t iSerialNumber;
};

#endif // USB_DEVICE_H
