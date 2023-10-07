#include "pch.h"

#include "Logger.hpp"

#include <iostream>
#include <cstdarg>

void Logger::Logger::Log(const char* format, ...)
{
	va_list args;
	va_start(args, format);

	std::lock_guard<std::mutex> guard(m_mutex);
	
	vprintf(format, args);

	va_end(args);
}