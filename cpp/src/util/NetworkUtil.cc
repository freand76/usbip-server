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

#include <string.h>
#include "NetworkUtil.h"

namespace NetworkUtil {
    unsigned int GetUint(uint8_t *buffer, int offset, int byteWidth) {
        unsigned int res = 0;
        for (int idx = 0; idx < byteWidth; idx++) {
            res = res << 8;
            res = res | buffer[offset + idx];
        }
        return res;
    };

    int SetUint(unsigned int value, uint8_t *buffer, int offset, int byteWidth) {
        if (buffer != NULL) {
            for (int idx = byteWidth - 1; idx >= 0; idx--) {
                buffer[offset + idx] = value >> ((byteWidth - idx - 1) * 8);
            }
        }
        return byteWidth;
    };

    int AddData(uint8_t *data, uint8_t *buffer, int offset, int len) {
        if (buffer != NULL) {
            memcpy(&buffer[offset], data, len);
        }
        return len;
    }
}; // namespace NetworkUtil
