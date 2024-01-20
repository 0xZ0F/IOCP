#pragma once

#ifndef NDEBUG
#include <iostream>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Logger/Logger.hpp"

#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
//#define DEBUG_PRINT(format, ...) fprintf(stderr, "[DEBUG] %s:%d:%s(): ", __FILENAME__, __LINE__, __func__); fprintf (stderr, format, ##__VA_ARGS__)
extern Logger::Logger _g_logger;
#define DEBUG_PRINT(format, ...) _g_logger.Log((std::string("[DEBUG] %s:%d:%s():TID %d: ") + std::string(format)).c_str(), __FILENAME__, __LINE__, __func__, GetCurrentThreadId(), ##__VA_ARGS__);

#else

#define DEBUG_PRINT(format, ...)

#endif