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

#include "Verbose.h"
#include <cstdarg>
#include <cstdio>

namespace Verbose {
    void ERROR(const char* format, ...) {
	va_list arglist;

	std::fprintf(stderr, "[ERROR] ");
	va_start( arglist, format );
	std::vfprintf(stderr, format, arglist );
	va_end( arglist );
	std::fprintf(stderr, "\n");
	std::fflush(stderr);
    }

    void INFO(const char* format, ...) {
	va_list arglist;

	std::printf("[INFO] ");
	va_start( arglist, format );
	std::vprintf(format, arglist );
	va_end( arglist );
	std::printf("\n");
    }

    void INFO_VECTOR(const char* name, unsigned char* vector, int size) {
	std::printf("[INFO] %s (%d):", name, size);
	for (int idx = 0; idx < size; idx++) {
	    if ((idx % 16) == 0) {
		std::printf("\n[INFO] ");
	    }
	    if ((idx % 16) == 8) {
		std::printf(" ");
	    }
	    std::printf("%.2x ", vector[idx]);
	}
	std::printf("\n");
    }
}
