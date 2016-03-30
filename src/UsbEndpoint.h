#ifndef USB_ENDPOINT_H
#define USB_ENDPOINT_H

#include <stdint.h>

class UsbEndpoint {
public:
    UsbEndpoint(uint8_t bEndpointAddress,
                uint8_t bmAttributes,
                uint16_t wMaxPacketSize,
                uint8_t bInterval);
    int GenerateDescriptor(unsigned char* buffer, int offset);

    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
};

#endif // USB_ENDPOINT_H
