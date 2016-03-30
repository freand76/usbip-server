#include "UsbConfiguration.h"
#include "UsbUtil.h"

using namespace UsbUtil;


UsbConfiguration::UsbConfiguration(uint8_t bNumInterfaces,
				   uint8_t bConfigurationValue,
				   uint8_t iConfiguration,
				   uint8_t bmAttributes,
				   uint8_t bMaxPower,
				   UsbInterface** interfaceArray) {
    this->bNumInterfaces = bNumInterfaces;
    this->bConfigurationValue = bConfigurationValue;
    this->iConfiguration = iConfiguration;
    this->bmAttributes = bmAttributes;
    this->bMaxPower = bMaxPower;
    this->interfaceArray = interfaceArray;
}

int UsbConfiguration::GenerateDescriptor(unsigned char* buffer, int offset) {
    buffer[0] = 9;
    buffer[1] = 2;
    buffer[4] = bNumInterfaces;
    buffer[5] = bConfigurationValue;
    buffer[6] = iConfiguration;
    buffer[7] = bmAttributes;
    buffer[8] = bMaxPower;

    int pos = 9;
    for (int idx = 0; idx < bNumInterfaces; idx++) {
	pos += interfaceArray[idx]->GenerateDescriptor(buffer, offset + pos);
    }

    /* Set total size */
    SetUint(pos, buffer, 2, 2);
    return pos;
}
