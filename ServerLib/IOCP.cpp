#include "DebugPrint.h"

#include <functional>

#include "IOCP.hpp"
#include "WSAOVERLAPPEDPLUS.h"

#include <MSWSock.h>

#pragma comment (lib, "Ws2_32.lib")

#ifndef NDEBUG
Logger::Logger _g_IOCPLog{};
#endif

IOCP::IOCP::IOCP(MoreDataCb_t MoreDataCb, ProcessPacketCb_t ProcessPacketCb) :
	m_hIOCP(UniqueHandle(NULL, CloseHandle)),
	m_hShutdownEvent(UniqueHandle(NULL, CloseHandle)),
	m_MoreDataCb(MoreDataCb),
	m_ProcessPacketCb(ProcessPacketCb)
{
	if(!m_MoreDataCb)
	{
		m_MoreDataCb = std::bind(&IOCP::DefaultMoreDataCb, this, std::placeholders::_1, std::placeholders::_2);
	}

	if(!m_ProcessPacketCb)
	{
		m_ProcessPacketCb = std::bind(&IOCP::DefaultProcessPacketCb, this, std::placeholders::_1);
	}
}

IOCP::IOCP::~IOCP()
{
	if(SetEvent(m_hShutdownEvent.get()))
	{
		for(auto& thread : m_vThreads)
		{
			thread.join();
		}
	}
	else
	{
		DEBUG_PRINT("SetEvent(m_hShutdownEvent.get()) %d\n", GetLastError());
	}
}

bool IOCP::IOCP::Begin(USHORT usPort, UINT8 uInitialWorkerThreads)
{
	m_hShutdownEvent = { CreateEventW(NULL, TRUE, FALSE, NULL), CloseHandle };
	if(!m_hShutdownEvent.get())
	{
		return false;
	}

	WSADATA wsaData = { 0 };
	if(NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		return false;
	}

	m_hIOCP = UniqueHandle(
		CreateIoCompletionPort(
			INVALID_HANDLE_VALUE,
			NULL,
			0,
			0),
		CloseHandle);

	if(!m_hIOCP)
	{
		return false;
	}

	for(int x = 0; x < uInitialWorkerThreads; x++)
	{
		IOCPThreadInfo threadInfo;
		threadInfo.pParam = IntToPtr(x + 1);

		std::thread newThread(&IOCP::WorkerThread, this, std::move(threadInfo));
		m_vThreads.push_back(std::move(newThread));
	}

	if(!CreateListeningSocket(usPort))
	{
		return false;
	}

	if(!ScheduleAccept())
	{
		return false;
	}

	return true;
}

bool IOCP::IOCP::CreateListeningSocket(USHORT usPort)
{
	if(!m_hListenSocket.CreateSocketW(
		AF_INET,
		SOCK_STREAM,
		0,
		NULL,
		0,
		WSA_FLAG_OVERLAPPED))
	{
		return false;
	}

	std::shared_ptr<IOCPContext> pListenContext = m_contextManager.CreateContext();
	pListenContext->SetSocket(m_hListenSocket.GetSocket());
	pListenContext->OpCode = IOCPContext::OP_LISTEN;
	AssociateWithIOCP(pListenContext.get());

	struct sockaddr_in servAddr = { 0 };
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = INADDR_ANY;
	servAddr.sin_port = htons(usPort);
	if(SOCKET_ERROR == bind(m_hListenSocket.GetSocket(), (struct sockaddr*)&servAddr, sizeof(servAddr)))
	{
		return false;
	}

	if(SOCKET_ERROR == listen(m_hListenSocket.GetSocket(), SOMAXCONN))
	{
		return false;
	}

	return true;
}

bool IOCP::IOCP::ScheduleAccept()
{
	// Create a new listening socket
	DWORD dwBytes;
	SOCKET preemptiveSocket;
	LPFN_ACCEPTEX lpfnAcceptEx;
	GUID guidAcceptEx = WSAID_ACCEPTEX;
	WSAOVERLAPPEDPLUS* pOverlappedPlus = AllocWSAOverlappedPlus((sizeof(SOCKADDR_IN) + 16) * 2); // See MSDN for size.

	if(!pOverlappedPlus)
	{
		return false;
	}

	preemptiveSocket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if(INVALID_SOCKET == preemptiveSocket)
	{
		//WriteToConsole(stderr, "WSASocketW() %d\n", WSAGetLastError());
		delete pOverlappedPlus;
		return FALSE;
	}

	if(SOCKET_ERROR == WSAIoctl(m_hListenSocket.GetSocket(),
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidAcceptEx,
		sizeof(guidAcceptEx),
		&lpfnAcceptEx,
		sizeof(lpfnAcceptEx),
		&dwBytes,
		NULL,
		NULL
	))
	{
		//WriteToConsole(stderr, "WSAIoctl() %d\n", WSAGetLastError());
		FreeWSAOverlappedPlus(&pOverlappedPlus);
		return false;
	}

	pOverlappedPlus->hClientSocket = preemptiveSocket;
	pOverlappedPlus->hListenSocket = m_hListenSocket.GetSocket();

	if(FALSE == lpfnAcceptEx(
		m_hListenSocket.GetSocket(),
		preemptiveSocket,
		pOverlappedPlus->pAcceptBuf,
		0,
		sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16,
		&pOverlappedPlus->dwBytesReceived,
		&pOverlappedPlus->wsaOverlapped))
	{
		if(ERROR_IO_PENDING != WSAGetLastError())
		{
			//WriteToConsole(stderr, "AcceptEx() %d\n", WSAGetLastError());
			FreeWSAOverlappedPlus(&pOverlappedPlus);
			return false;
		}
	}

	return true;
}

bool IOCP::IOCP::HandleNewConnection(SOCKET clientListenSock)
{
	// Create context for new client
	auto pContext = std::make_shared<IOCPContext>();
	pContext->OpCode = IOCPContext::OP_WRITE;
	pContext->SetSocket(clientListenSock);

	m_contextManager.AddContext(pContext);

	if(FALSE == AssociateWithIOCP(pContext.get()))
	{
		DEBUG_PRINT("AssociateWithIOCP()\n");
		m_contextManager.RemoveContext(pContext);
		return false;
	}

	if(!pContext->ScheduleRecv())
	{
		DEBUG_PRINT("ScheduleRecv()\n");
		m_contextManager.RemoveContext(pContext);
		return false;
	}

	if(!ScheduleAccept())
	{
		DEBUG_PRINT("ScheduleAccept()\n");
		m_contextManager.RemoveContext(pContext);
		return false;
	}

	return true;
}

bool IOCP::IOCP::AssociateWithIOCP(const IOCPContext* pContext)
{
	SOCKET sock = pContext->GetSocketCopy();

	// Associate the socket with IOCP
	HANDLE hTemp = CreateIoCompletionPort((HANDLE)sock, m_hIOCP.get(), (ULONG_PTR)pContext, 0);
	if(NULL == hTemp)
	{
		// TODO: Remove Context
		return false;
	}
	return true;
}

bool IOCP::IOCP::WorkerThread(IOCPThreadInfo&& threadInfo)
{
	DEBUG_PRINT("Thread %d Started\n", GetCurrentThreadId());
	INT nBytesSent = 0;
	DWORD dwBytes = 0;
	DWORD dwBytesTransfered = 0;
	UINT_PTR nThreadNo = PtrToLong(threadInfo.pParam);
	OVERLAPPED* pOverlapped = NULL;
	IOCPContext* pContext = NULL;
	WSAOVERLAPPEDPLUS* pOverlappedPlus = NULL;
	SOCKET targetSock = INVALID_SOCKET;

	while(WAIT_OBJECT_0 != WaitForSingleObject(m_hShutdownEvent.get(), 0))
	{
		BOOL fReturn = GetQueuedCompletionStatus(
			m_hIOCP.get(),
			&dwBytesTransfered,
			(PULONG_PTR)&pContext,
			&pOverlapped,
			100
		);

		// https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-getqueuedcompletionstatus
		if(!fReturn)
		{
			if(!pOverlapped)
			{
				if(WAIT_TIMEOUT != GetLastError())
				{
					// ERROR_ABANDONED_WAIT_0
					break;
				}
				continue;
			}

			if(!pContext)
			{
				break;
			}
		}

		// Client disconnect check
		if(pContext->OpCode != IOCPContext::OP_READ && m_hListenSocket.GetSocket() != pContext->GetSocketCopy())
		{
			if((!fReturn) || (fReturn && !dwBytesTransfered))
			{
				// Client disconnected
				DEBUG_PRINT("Client Disconnected\n");
				m_contextManager.RemoveContext(pContext);
				continue;
			}
		}

		int nBytesRecv = 0;
		DWORD dwFlags = 0;
		WSAOVERLAPPED* p_ol = pContext->GetOverlapped();
		std::string data{};
		size_t stDataLeft = 0;
		switch(pContext->OpCode)
		{
		case IOCPContext::OP_ACCEPT:
			break;

		case IOCPContext::OP_LISTEN:
			// New connection
			DEBUG_PRINT("New Connection\n");
			pOverlappedPlus = CONTAINING_RECORD(pOverlapped, WSAOVERLAPPEDPLUS, wsaOverlapped);

			// Register new client
			if(!HandleNewConnection(pOverlappedPlus->hClientSocket))
			{
				DEBUG_PRINT("HandleNewConnection()\n");
				break;
			}

			if(!FreeWSAOverlappedPlus(&pOverlappedPlus))
			{
				DEBUG_PRINT("FreeWSAOverlappedPlus()\n");
			}

			break;

		case IOCPContext::OP_READ:
			// Client is reading, server is writing
			DEBUG_PRINT("Read %d bytes.\n", dwBytesTransfered);

			pContext->BytesSent += dwBytesTransfered;

			// If not all data has been sent
			if(pContext->BytesSent < pContext->BytesToSend)
			{
				if(!pContext->ScheduleSend(pContext->BytesSent))
				{
					DEBUG_PRINT("ScheduleSend() %d\n", WSAGetLastError());
					m_contextManager.RemoveContext(pContext);
					break;
				}
			}
			else // All data sent, post recv
			{
				pContext->Reset();
				if(!pContext->ScheduleRecv())
				{
					DEBUG_PRINT("pContext->ScheduleRecv()() %d\n", WSAGetLastError());
					m_contextManager.RemoveContext(pContext);
				}
				pContext->OpCode = IOCPContext::OP_WRITE;
			}

			break;

		case IOCPContext::OP_WRITE:
			// Client Has Written

			DEBUG_PRINT("Client writing.\n");

			pContext->GetBufferContents(data);
			pContext->BytesRecvd += dwBytesTransfered;
			data.resize(pContext->BytesRecvd);

			if(pContext->BytesToRecv > 0 && (pContext->BytesToRecv < pContext->BytesRecvd))
			{
				pContext->ScheduleRecv(pContext->BytesRecvd, pContext->BytesToRecv - pContext->BytesRecvd);
				break;
			}

			if(m_MoreDataCb(data, stDataLeft))
			{
				pContext->BytesToRecv = pContext->BytesRecvd + (ULONG)stDataLeft;
				pContext->ScheduleRecv(pContext->BytesRecvd, (ULONG)stDataLeft);
				break;
			}

			pContext->SetBufferSize(pContext->BytesRecvd);

			pContext->BytesSent = 0;
			pContext->BytesToSend = 0;
			m_ProcessPacketCb(*pContext);

			pContext->BytesToRecv = 0;
			pContext->OpCode = IOCPContext::OP_READ;
			break;

		default:
			DEBUG_PRINT("Invaild Opcode\n");
			break;
		}
	}

	DEBUG_PRINT("Thread exiting.\n");
	return true;
}