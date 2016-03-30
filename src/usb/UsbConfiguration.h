#ifndef USB_CONFIGURATION_H
#define USB_CONFIGURATION_H

#include <stdint.h>
#include "UsbInterface.h"

class UsbConfiguration {
public:
    UsbConfiguration(uint8_t bNumInterfaces,
                     uint8_t bConfigurationValue,
                     uint8_t iConfiguration,
                     uint8_t bmAttributes,
                     uint8_t bMaxPower,
                     UsbInterface** interfaceArray);

    int GenerateDescriptor(unsigned char* buffer, int offset);

    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t iConfiguration;
    uint8_t bmAttributes;
    uint8_t bMaxPower;
    UsbInterface** interfaceArray;
};

#endif // USB_CONFIGURATION_H
