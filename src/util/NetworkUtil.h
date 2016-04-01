/*******************************************************
 usbip-server - a platform for USB device prototyping

 Fredrik Andersson
 Copyright 2016, All Rights Reserved.

 This software may be used by anyone for any reason so
 long as the copyright notice in the source files
 remains intact.

 code repository located at:
        http://github.com/freand76/usbip-server
********************************************************/

#ifndef NETWORK_UTIL_H
#define NETWORK_UTIL_H

#include <stdint.h>

namespace NetworkUtil {
    unsigned int GetUint(uint8_t* buffer, int offset, int byteWidth);
    int SetUint(unsigned int value, uint8_t* buffer, int offset, int byteWidth);
    int AddData(uint8_t* data, uint8_t* buffer, int offset, int len);
};

#endif // NETWORK_UTIL_H
