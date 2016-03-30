#ifndef USB_INTERFACE_H
#define USB_INTERFACE_H

#include <stdint.h>
#include "UsbEndpoint.h"

class UsbInterface {
public:
    UsbInterface(uint8_t bInterfaceNumber,
                 uint8_t bAlternateSetting,
                 uint8_t bNumEndpoints,
                 uint8_t bInterfaceClass,
                 uint8_t bInterfaceSubClass,
                 uint8_t bInterfaceProtocol,
                 uint8_t iInterface,
                 UsbEndpoint** endpointArray);

    int GenerateDescriptor(unsigned char* buffer, int offset);

    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
    UsbEndpoint** endpointArray;
};

#endif // USB_INTERFACE_H
