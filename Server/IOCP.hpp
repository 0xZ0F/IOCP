#pragma once

#include <memory>
#include <vector>
#include <thread>
#include <string_view>
#include <functional>

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
		/// <summary>
		/// Callback type used for determining how much more data needs to be read.
		/// </summary>
		using MoreDataCb_t = std::function<bool(const std::string_view& recvd, size_t& amountLeft)>;

		/// <summary>
		/// Callback type used for processing a packet.
		/// This function is called in a blocking manner, so spend as little time as possible in it.
		/// </summary>
		using ProcessPacketCb_t = std::function<bool(IOCPContext& context)>;

		using UniqueHandle = std::unique_ptr<void, decltype(&CloseHandle)>;
		using UniqueWSAEvent = std::unique_ptr<void, decltype(&WSACloseEvent)>;

	protected:
		IOCPContextManager m_contextManager;

		UniqueSocket m_hListenSocket;
		UniqueWSAEvent m_hAcceptEvent;
		UniqueHandle m_hShutdownEvent;
		UniqueHandle m_hIOCP;
		std::vector<std::thread> m_vThreads;

		MoreDataCb_t m_MoreDataCb;
		ProcessPacketCb_t m_ProcessPacketCb;

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
		bool DefaultMoreDataCb(const std::string_view& recvd, size_t& amountLeft);
		bool DefaultProcessPacketCb(IOCPContext& context);

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

