#pragma once

#include <memory>
#include <vector>
#include <thread>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>

#include "UniqueSocket.hpp"
#include "IOCPContextManager.hpp"

namespace IOCP
{
	class IOCPThreadInfo;

	class IOCP
	{
	public:
		using UniqueHandle = std::unique_ptr<void, decltype(&CloseHandle)>;
		using UniqueWSAEvent = std::unique_ptr<void, decltype(&WSACloseEvent)>;

	protected:
		UniqueSocket m_hListenSocket;
		UniqueWSAEvent m_hAcceptEvent;
		UniqueHandle m_hShutdownEvent;
		UniqueHandle m_hIOCP;
		std::vector<std::thread> m_vThreads;

		IOCPContextManager m_contextManager;

	public:
		IOCP();
		IOCP(const IOCP&) = delete;
		IOCP(IOCP&&) = delete;
		void operator=(const IOCP&) = delete;
		void operator=(IOCP&&) = delete;

		/// <summary>
		/// Begin accepting clients.
		/// </summary>
		/// <param name="usPort"></param>
		/// <returns></returns>
		bool Begin(unsigned short usPort);

	protected:
		bool CreateListeningSocket(unsigned short usPort);
		bool ScheduleAccept();
		bool HandleNewConnection(SOCKET clientListenSock);
		bool AssociateWithIOCP(const IOCPContext* pContext);
		bool WorkerThread(IOCPThreadInfo&& threadInfo);
	};

	class IOCPThreadInfo
	{
	public:
		LPVOID pParam;

		IOCPThreadInfo(IOCP* iocp = NULL, LPVOID param = NULL) :
			pParam(param)
		{}
	};
}

