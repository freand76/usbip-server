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
    static VerboseLogLevel_t logLevel = LEVEL_ERROR;

    void SetVerboseLevel(VerboseLogLevel_t level) {
	logLevel = level;
    }

    FILE* TYPE(VerboseLogLevel_t level) {
	if (logLevel < level) return NULL;

	FILE* fd = NULL;
	switch(level) {
	case LEVEL_DEBUG:
	    fd = stdout;
	    std::fprintf(fd, "[DEBUG] ");
	    break;
	case LEVEL_INFO:
	    fd = stdout;
	    std::fprintf(fd, "[INFO] ");
	    break;
	case LEVEL_ERROR:
	    fd = stderr;
	    std::fprintf(fd, "[ERROR] ");
	    break;
	}
	return fd;
    }

    void TEXT(VerboseLogLevel_t level, const char* format, va_list arglist) {
	FILE* fd = TYPE(level);
	if (fd == NULL) return;

	std::vfprintf(fd, format, arglist );
	std::fprintf(fd, "\n");
	std::fflush(fd);
    }

    void VECTOR(VerboseLogLevel_t level, const char* name, uint8_t* vector, int size) {
	FILE* fd = TYPE(level);
	if (fd == NULL) return;

	std::fprintf(fd, "%s (%d):", name, size);
	for (int idx = 0; idx < size; idx++) {
	    if ((idx % 16) == 0) {
		std::fprintf(fd, "\n");
		TYPE(level);
	    }
	    if ((idx % 16) == 8) {
		std::fprintf(fd, " ");
	    }
	    std::fprintf(fd, "%.2x ", vector[idx]);
	}
	std::fprintf(fd, "\n");
	std::fflush(fd);

    }

    void ERROR(const char* format, ...) {
	va_list arglist;
	va_start( arglist, format );
	TEXT(LEVEL_ERROR, format, arglist);
	va_end( arglist );
    }

    void INFO(const char* format, ...) {
	va_list arglist;
	va_start( arglist, format );
	TEXT(LEVEL_INFO, format, arglist);
	va_end( arglist );
    }

    void DEBUG(const char* format, ...) {
	va_list arglist;
	va_start( arglist, format );
	TEXT(LEVEL_DEBUG, format, arglist);
	va_end( arglist );
    }

    void ERROR_VECTOR(const char* name, uint8_t* vector, int size) {
	VECTOR(LEVEL_ERROR, name, vector, size);
    }

    void INFO_VECTOR(const char* name, uint8_t* vector, int size) {
	VECTOR(LEVEL_INFO, name, vector, size);
    }

    void DEBUG_VECTOR(const char* name, uint8_t* vector, int size) {
	VECTOR(LEVEL_DEBUG, name, vector, size);
    }

}
