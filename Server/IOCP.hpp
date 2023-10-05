#pragma once

#include <memory>
#include <vector>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>

#include "UniqueSocket.hpp"
#include "IOCPContextManager.hpp"

namespace IOCP
{
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
		std::vector<UniqueHandle> m_vThreads;

		IOCPContextManager m_contextManager;

	public:
		IOCP();
		IOCP(const IOCP&) = delete;
		IOCP(IOCP&&) = delete;
		void operator=(const IOCP&) = delete;
		void operator=(IOCP&&) = delete;

		/// <summary>
		/// Initialize the IOCP class.
		/// </summary>
		/// <returns></returns>
		bool Init();

		/// <summary>
		/// Begin accepting clients.
		/// </summary>
		/// <param name="usPort"></param>
		/// <returns></returns>
		bool Begin(u_short usPort);

	protected:
		bool ScheduleAccept();
		bool AcceptConnection(SOCKET clientListenSock);
		static DWORD WINAPI WorkerThread(PVOID _pThreadInfo);
		bool AssociateWithIOCP(const IOCPContext* pContext);
	};

	class IOCPThreadInfo
	{
	public:
		std::shared_ptr<IOCP> pIOCP;
		LPVOID pParam;

		IOCPThreadInfo(IOCP* iocp = NULL, LPVOID param = NULL) :
			pIOCP(iocp), pParam(param)
		{}
	};
}

