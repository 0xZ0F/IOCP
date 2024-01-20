#pragma once

#ifndef NDEBUG

#include <iostream>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Logger/Logger.hpp"

extern Logger::Logger _g_IOCPLog; // In IOCP.cpp
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#define DEBUG_PRINT(format, ...) _g_IOCPLog.Log((std::string("[DEBUG] %s:%d:%s():TID %d: ") + std::string(format)).c_str(), __FILENAME__, __LINE__, __func__, GetCurrentThreadId(), ##__VA_ARGS__);

#else

#define DEBUG_PRINT(format, ...)

#endif