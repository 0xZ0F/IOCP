#pragma once

#include <mutex>
#include <queue>
#include <string>

namespace Logger
{
	class Logger
	{
	private:
		inline static std::mutex m_mutex;
		inline static std::queue<std::string> m_queue;

	public:
		void Log(const char* format, ...);
	};
}
