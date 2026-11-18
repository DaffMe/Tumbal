#pragma once
// Helper format string gaya printf → std::string.

#include <cstdarg>
#include <cstdio>
#include <string>

inline std::string strf(const char* f, ...) __attribute__((format(printf, 1, 2)));
inline std::string strf(const char* f, ...) {
    char buf[256];
    va_list args;
    va_start(args, f);
    std::vsnprintf(buf, sizeof buf, f, args);
    va_end(args);
    return buf;
}
