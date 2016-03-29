#ifndef USB_DEVICE_H
#define USB_DEVICE_H

#include <stdint.h>
#include "UsbConfiguration.h"

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

    int TxRx(unsigned char* setup, unsigned char* data, unsigned char* replyBuffer, int bufLength);
    int OutRequest(unsigned char* setup, unsigned char* data, unsigned char* replyBuffer, int bufLength);
    int InRequest(unsigned char* setup, unsigned char* data, unsigned char* replyBuffer, int bufLength);
    int DeviceRequest(unsigned char* setup, unsigned char* data, unsigned char* replyBuffer, int bufLength);
    
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
