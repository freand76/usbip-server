#include "UsbEndpoint.h"
#include "UsbUtil.h"

using namespace UsbUtil;

UsbEndpoint::UsbEndpoint(int bEndpointAddress,
			 int bmAttributes,
			 int wMaxPacketSize,
			 int bInterval) {
    this->bEndpointAddress = bEndpointAddress;
    this->bmAttributes  = bmAttributes;
    this->wMaxPacketSize = wMaxPacketSize;
    this->bInterval = bInterval; 
}

int UsbEndpoint::GenerateConfigurationData(unsigned char* buffer, int offset) {
    buffer[offset + 0] = 7;
    buffer[offset + 1] = 5;
    buffer[offset + 2] = bEndpointAddress;
    buffer[offset + 3] = bmAttributes;
    SetUint(wMaxPacketSize, buffer, offset + 4, 2);
    buffer[offset + 6] = bInterval;
    return 7;
}
