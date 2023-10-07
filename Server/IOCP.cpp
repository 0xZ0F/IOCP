#include "DebugPrint.h"

#include <functional>

#include "IOCP.hpp"
#include "WSAOVERLAPPEDPLUS.h"

#include <MSWSock.h>

#pragma comment (lib, "Ws2_32.lib")

IOCP::IOCP::IOCP() :
	m_hIOCP(UniqueHandle(NULL, CloseHandle)),
	m_hShutdownEvent(UniqueHandle(NULL, CloseHandle)),
	m_hAcceptEvent(UniqueWSAEvent(NULL, WSACloseEvent))
{}

bool IOCP::IOCP::Begin(unsigned short usPort)
{
	// Initialize Winsock
	WSADATA wsaData = { 0 };
	if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		return false;
	}

	//Create I/O completion port
	m_hIOCP = UniqueHandle(
		CreateIoCompletionPort(
			INVALID_HANDLE_VALUE,
			NULL,
			0,
			0),
		CloseHandle);

	if (!m_hIOCP)
	{
		return false;
	}

	//Create 3 worker threads to start
	for (int x = 0; x < 3; x++)
	{
		IOCPThreadInfo threadInfo;
		threadInfo.pParam = IntToPtr(x + 1);

		std::thread newThread(&IOCP::WorkerThread, this, std::move(threadInfo));
		m_vThreads.push_back(std::move(newThread));
	}

	if (!CreateListeningSocket(usPort))
	{
		return false;
	}

	if (!ScheduleAccept())
	{
		return false;
	}

	return true;
}

bool IOCP::IOCP::CreateListeningSocket(unsigned short usPort)
{
	if (!m_hListenSocket.CreateSocketW(
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
	pListenContext->SetOpCode(IOCPContext::OP_LISTEN);
	AssociateWithIOCP(pListenContext.get());

	struct sockaddr_in servAddr = { 0 };
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = INADDR_ANY;
	servAddr.sin_port = htons(usPort);
	if (SOCKET_ERROR == bind(m_hListenSocket.GetSocket(), (struct sockaddr*)&servAddr, sizeof(servAddr)))
	{
		return false;
	}

	if (SOCKET_ERROR == listen(m_hListenSocket.GetSocket(), SOMAXCONN))
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

	if (!pOverlappedPlus)
	{
		return false;
	}

	preemptiveSocket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == preemptiveSocket)
	{
		//WriteToConsole(stderr, "WSASocketW() %d\n", WSAGetLastError());
		delete pOverlappedPlus;
		return FALSE;
	}

	if (SOCKET_ERROR == WSAIoctl(m_hListenSocket.GetSocket(),
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

	if (FALSE == lpfnAcceptEx(
		m_hListenSocket.GetSocket(),
		preemptiveSocket,
		pOverlappedPlus->pAcceptBuf,
		0,
		sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16,
		&pOverlappedPlus->dwBytesReceived,
		&pOverlappedPlus->wsaOverlapped))
	{
		if (ERROR_IO_PENDING != WSAGetLastError())
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
	pContext->SetOpCode(IOCPContext::OP_WRITE);
	pContext->SetSocket(clientListenSock);

	m_contextManager.AddContext(pContext);

	if (FALSE == AssociateWithIOCP(pContext.get()))
	{
		/*WriteToConsole(stderr, "AssociateWithIOCP()\n");
		RemoveFromClientListAndFreeMemory(pContext);*/
		return false;
	}

	if (!pContext->ScheduleRecv())
	{
		return false;
	}

	if (!ScheduleAccept())
	{
		return false;
	}

	return true;
}

bool IOCP::IOCP::AssociateWithIOCP(const IOCPContext* pContext)
{
	SOCKET sock = pContext->GetSocketCopy();

	// Associate the socket with IOCP
	HANDLE hTemp = CreateIoCompletionPort((HANDLE)sock, m_hIOCP.get(), (ULONG_PTR)pContext, 0);
	if (NULL == hTemp)
	{
		// TODO: Remove Context
		return false;
	}
	return true;
}

bool IOCP::IOCP::WorkerThread(IOCPThreadInfo&& threadInfo)
{
	DEBUG_PRINT("Thread %d Created\n", GetCurrentThreadId());
	INT nBytesSent = 0;
	DWORD dwBytes = 0;
	DWORD dwBytesTransfered = 0;
	UINT_PTR nThreadNo = PtrToLong(threadInfo.pParam);
	OVERLAPPED* pOverlapped = NULL;
	IOCPContext* pContext = NULL;
	WSAOVERLAPPEDPLUS* pOverlappedPlus = NULL;
	SOCKET targetSock = INVALID_SOCKET;

	while (WAIT_OBJECT_0 != WaitForSingleObject(m_hShutdownEvent.get(), 0))
	{
		BOOL bReturn = GetQueuedCompletionStatus(
			m_hIOCP.get(),
			&dwBytesTransfered,
			(PULONG_PTR)&pContext,
			&pOverlapped,
			INFINITE
		);

		// This catches errors and the destructor
		if (NULL == pContext)
		{
			break;
		}

		// Client disconnect check
		if (pContext->GetOpCode() != IOCPContext::OP_READ && m_hListenSocket.GetSocket() != pContext->GetSocketCopy())
		{
			if ((FALSE == bReturn) || ((TRUE == bReturn) && (0 == dwBytesTransfered)))
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

		switch (pContext->GetOpCode())
		{
		case IOCPContext::OP_ACCEPT:
			break;

		case IOCPContext::OP_LISTEN:
			// New connection
			DEBUG_PRINT("New Connection\n");
			pOverlappedPlus = CONTAINING_RECORD(pOverlapped, WSAOVERLAPPEDPLUS, wsaOverlapped);

			// Copy listening socket properties onto connected client socket. See MSDN.
			//if (setsockopt(
			//	pContext->GetSocketCopy(),
			//	SOL_SOCKET,
			//	SO_UPDATE_ACCEPT_CONTEXT,
			//	(char*)&m_hListenSocket,
			//	sizeof(m_hListenSocket)))
			//{
			//	DEBUG_PRINT("setsockopt() %d\n", WSAGetLastError());
			//	
			//	// Remove Client
			//	// Remove Client
			//	// Remove Client
			//	// Remove Client
			//	// Remove Client

			//	if (!FreeWSAOverlappedPlus(&pOverlappedPlus))
			//	{
			//		DEBUG_PRINT("FreeWSAOverlappedPlus()\n");
			//	}

			//	continue;
			//}
			
			// Register new client
			if (!HandleNewConnection(pOverlappedPlus->hClientSocket))
			{
				DEBUG_PRINT("HandleNewConnection()\n");
				break;
			}
			
			if (!FreeWSAOverlappedPlus(&pOverlappedPlus))
			{
				DEBUG_PRINT("FreeWSAOverlappedPlus()\n");
			}

			break;

		case IOCPContext::OP_READ:
			// Client is reading, server is writing
			DEBUG_PRINT("Read %d bytes.\n", dwBytesTransfered);
			pContext->IncSentBytes(dwBytesTransfered);

			// If not all data has been sent
			if (pContext->GetSentBytes() < pContext->GetTotalBytes())
			{
				/*pContext->SetOpCode(IOCPContext::OP_READ);

				p_wbuf->buf += pContext->GetSentBytes();
				p_wbuf->len = pContext->GetTotalBytes() - pContext->GetSentBytes();
				dwFlags = 0;
				nBytesSent = WSASend(pContext->GetSocket(), p_wbuf, 1, &dwBytes, dwFlags, p_ol, NULL);

				if ((SOCKET_ERROR == nBytesSent) && (WSA_IO_PENDING != WSAGetLastError()))
				{
					DEBUG_PRINT("WSASend() %d\n", WSAGetLastError());
					m_contextManager.RemoveContext(pContext);
				}*/
			}
			// All data sent, post recv
			else
			{
				// Free WSABuf here
				if (!pContext->ScheduleRecv())
				{
					DEBUG_PRINT("pContext->ScheduleRecv()() %d\n", WSAGetLastError());
					m_contextManager.RemoveContext(pContext);
				}
				pContext->SetOpCode(IOCPContext::OP_WRITE);
			}
			
			break;

		case IOCPContext::OP_WRITE:
			// Client is writing, server is reading, send response (echo)
			DEBUG_PRINT("Client writing.\n");
			char szBuffer[MAX_BUFFER_LEN];

			// Recving data is handled by accept thread
			if (!pContext->GetBuffer(szBuffer, sizeof(szBuffer)))
			{
				DEBUG_PRINT("CClientContext->GetBuffer()\n");
				break;
			}

			//WriteToConsole(stdout, "Thread %d: Message: %s\n", nThreadNo, (char*)pkt.GetDataPointer());

			// Send the message back to the client.
			pContext->SetOpCode(IOCPContext::OP_READ);
			pContext->SetTotalBytes(dwBytesTransfered);
			pContext->SetSentBytes(0);
			/*p_wbuf->len = dwBytesTransfered;
			dwFlags = 0;
			nBytesSent = WSASend(targetSock, p_wbuf, 1, &dwBytes, dwFlags, p_ol, NULL);*/

			pContext->ScheduleSend();

			if ((SOCKET_ERROR == nBytesSent) && (WSA_IO_PENDING != WSAGetLastError()))
			{
				DEBUG_PRINT("WSASend() %d\n", WSAGetLastError());
				m_contextManager.RemoveContext(pContext);
			}

			break;

		default:
			DEBUG_PRINT("Invaild Opcode\n");
			break;
		}
		
		DEBUG_PRINT("Thread exiting.\n");
	}

	return true;
}
