#ifndef USB_INTERFACE_H
#define USB_INTERFACE_H

#include "UsbEndpoint.h"

class UsbInterface {
public:
    UsbInterface(int bInterfaceNumber,
                 int bAlternateSetting,
                 int bNumEndpoints,
                 int bInterfaceClass,
                 int bInterfaceSubClass,
                 int bInterfaceProtocol,
                 int iInterface,
                 UsbEndpoint** endpointArray);

    int GenerateConfigurationData(unsigned char* buffer, int offset);

    int bInterfaceNumber;
    int bAlternateSetting;
    int bNumEndpoints;
    int bInterfaceClass;
    int bInterfaceSubClass;
    int bInterfaceProtocol;
    int iInterface;
    UsbEndpoint** endpointArray;
};

#endif // USB_INTERFACE_H
