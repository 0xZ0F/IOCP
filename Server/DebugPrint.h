#pragma once

#include <cstdio>

#ifndef NDEBUG
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#define DEBUG_PRINT(format, ...) fprintf(stderr, "[DEBUG] %s:%d:%s(): ", __FILENAME__, __LINE__, __func__); fprintf (stderr, format, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(format, ...)
#endif