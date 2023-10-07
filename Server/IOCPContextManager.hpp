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

		/// <summary>
		/// Create a new context and add it to the list of tracked contexts.
		/// </summary>
		/// <returns>Returns a shared pointer to the newly created context.</returns>
		std::shared_ptr<IOCPContext> CreateContext();
		
		/// <summary>
		/// Add a context to the context list. Note: CreateContext() does this automatically.
		/// </summary>
		/// <param name="pContext"></param>
		void AddContext(std::shared_ptr<IOCPContext> pContext);
		
		/// <summary>
		/// Remove a context from the tracked list of contexts.
		/// </summary>
		/// <param name="pContext"></param>
		/// <returns>Returns true on success, false otherwise.</returns>
		bool RemoveContext(std::shared_ptr<IOCPContext> pContext);
		bool RemoveContext(IOCPContext* pContext);
	};
}

