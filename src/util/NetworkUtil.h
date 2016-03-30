#ifndef NETWORK_UTIL_H
#define NETWORK_UTIL_H

namespace NetworkUtil {
    unsigned int GetUint(unsigned char* buffer, int offset, int byteWidth);
    int SetUint(unsigned int value, unsigned char* buffer, int offset, int byteWidth);
    int AddData(unsigned char* data, unsigned char* buffer, int offset, int len);
};

#endif // NETWORK_UTIL_H
