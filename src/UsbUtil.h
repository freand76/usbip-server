#ifndef USB_UTIL_H
#define USB_UTIL_H

namespace UsbUtil {
    int SetUint(unsigned int value, unsigned char* buffer, int offset, int byteWidth);
};

#endif // USB_UTIL_H
