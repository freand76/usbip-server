#ifndef VERBOSE_H
#define VERBOSE_H

#include <string>

namespace Verbose {
    void ERROR(const char* format, ...);
    void INFO(const char* format, ...);
    void INFO_VECTOR(const char* name, unsigned char* vector, int size);
};

#endif // VERBOSE_H
