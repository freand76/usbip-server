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

#include "UsbUtil.h"

namespace UsbUtil {
    int SetUint(unsigned int value, unsigned char* buffer, int offset, int byteWidth) {
	int val = value;
	for (int idx = 0; idx < byteWidth; idx++) {
	    buffer[offset + idx] = val & 0xff;
	    val = val >> 8;
	}
	return byteWidth;
    }
}
