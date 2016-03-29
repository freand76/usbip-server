#ifndef USB_ENDPOINT_H
#define USB_ENDPOINT_H

class UsbEndpoint {
public:
    UsbEndpoint(int bEndpointAddress,
                int bmAttributes,
                int wMaxPacketSize,
                int bInterval);
    int GenerateConfigurationData(unsigned char* buffer, int offset);

    int bEndpointAddress;
    int bmAttributes;
    int wMaxPacketSize;
    int bInterval;
};

#endif // USB_ENDPOINT_H
