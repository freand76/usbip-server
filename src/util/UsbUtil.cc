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
