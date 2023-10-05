#pragma once

#include <memory>
#include <vector>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "IOCPContext.hpp"

namespace IOCP
{
	class IOCPContextManager
	{
	protected:
		CRITICAL_SECTION m_cs;
		std::vector<std::shared_ptr<IOCPContext>> m_vContexts;

	public:
		IOCPContextManager();
		~IOCPContextManager();

		std::weak_ptr<IOCPContext> AddContext(std::shared_ptr<IOCPContext> pContext);
		bool RemoveContext(std::shared_ptr<IOCPContext> pContext);
	};
}

