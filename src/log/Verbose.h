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

#ifndef VERBOSE_H
#define VERBOSE_H

#include <string>

namespace Verbose {
    typedef enum { LEVEL_ERROR = 0, LEVEL_INFO, LEVEL_DEBUG } VerboseLogLevel_t;

    void SetVerboseLevel(VerboseLogLevel_t level);
    void ERROR(const char *format, ...);
    void ERROR_VECTOR(const char *name, uint8_t *vector, int size);
    void INFO(const char *format, ...);
    void INFO_VECTOR(const char *name, uint8_t *vector, int size);
    void DEBUG(const char *format, ...);
    void DEBUG_VECTOR(const char *name, uint8_t *vector, int size);
}; // namespace Verbose

#endif // VERBOSE_H
