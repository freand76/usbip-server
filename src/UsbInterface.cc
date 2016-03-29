#include "UsbEndpoint.h"
#include "UsbInterface.h"

UsbInterface::UsbInterface(int bInterfaceNumber,
			   int bAlternateSetting,
			   int bNumEndpoints,
			   int bInterfaceClass,
			   int bInterfaceSubClass,
			   int bInterfaceProtocol,
			   int iInterface,
			   UsbEndpoint** endpointArray) {
    this->bInterfaceNumber = bInterfaceNumber;
    this->bAlternateSetting = bAlternateSetting;
    this->bNumEndpoints = bNumEndpoints;
    this->bInterfaceClass = bInterfaceClass;
    this->bInterfaceSubClass = bInterfaceSubClass;
    this->bInterfaceProtocol = bInterfaceProtocol;
    this->iInterface = iInterface;
    this->endpointArray = endpointArray;
}

int UsbInterface::GenerateConfigurationData(unsigned char* buffer, int offset) {
    buffer[offset + 0] = 9;
    buffer[offset + 1] = 4;
    buffer[offset + 2] = bInterfaceNumber;
    buffer[offset + 3] = bAlternateSetting;
    buffer[offset + 4] = bNumEndpoints;
    buffer[offset + 5] = bInterfaceClass;
    buffer[offset + 6] = bInterfaceSubClass;
    buffer[offset + 7] = bInterfaceProtocol;
    buffer[offset + 8] = iInterface;

    int pos = 9;
    for (int idx = 0; idx < bNumEndpoints; idx++) {
	pos += endpointArray[idx]->GenerateConfigurationData(buffer, offset + pos);
    }
    return pos;
}
