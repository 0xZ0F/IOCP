#include "IOCP.hpp"
#include "WSAOVERLAPPEDPLUS.h"

#include <MSWSock.h>

#pragma comment (lib, "Ws2_32.lib")

IOCP::IOCP::IOCP() :
	m_hIOCP(UniqueHandle(NULL, CloseHandle)),
	m_hShutdownEvent(UniqueHandle(NULL, CloseHandle)),
	m_hAcceptEvent(UniqueWSAEvent(NULL, WSACloseEvent))
{}

bool IOCP::IOCP::Init()
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

	//Create worker threads
	DWORD dwThreadID;
	for (int x = 0; x < m_vThreads.size(); x++)
	{
		IOCPThreadInfo* threadInfo = new IOCPThreadInfo(this, IntToPtr(x + 1));
		UniqueHandle hNewThread = UniqueHandle(
			CreateThread(
				0,
				0,
				WorkerThread,
				threadInfo,
				0,
				&dwThreadID),
			CloseHandle
		);

		if (!hNewThread)
		{
			return false;
		}

		m_vThreads.push_back(std::move(hNewThread));
	}

	return true;
}

bool IOCP::IOCP::Begin(u_short usPort)
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

	auto pListenContext = std::shared_ptr<IOCPContext>();
	pListenContext->SetSocket(m_hListenSocket.GetCopy());
	pListenContext->SetOpCode(IOCPContext::OP_LISTEN);
	m_contextManager.AddContext(pListenContext);
	AssociateWithIOCP(pListenContext.get());

	struct sockaddr_in servAddr = { 0 };
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = INADDR_ANY;
	servAddr.sin_port = htons(usPort);
	if (SOCKET_ERROR == bind(m_hListenSocket.GetCopy(), (struct sockaddr*)&servAddr, sizeof(servAddr)))
	{
		return false;
	}

	if (SOCKET_ERROR == listen(m_hListenSocket.GetCopy(), SOMAXCONN))
	{
		return false;
	}

	if (!ScheduleAccept())
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
	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	LPWSAOVERLAPPEDPLUS p_olp = new WSAOVERLAPPEDPLUS;
	int buflen = (sizeof(SOCKADDR_IN) + 16) * 2;
	char* pBuf = new char[buflen];

	ZeroMemory(pBuf, buflen);
	ZeroMemory(p_olp, sizeof(*p_olp));

	preemptiveSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == preemptiveSocket)
	{
		//WriteToConsole(stderr, "WSASocket() %d\n", WSAGetLastError());
		delete p_olp;
		return FALSE;
	}

	if (SOCKET_ERROR == WSAIoctl(m_hListenSocket.GetCopy(),
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidAcceptEx,
		sizeof(GuidAcceptEx),
		&lpfnAcceptEx,
		sizeof(lpfnAcceptEx),
		&dwBytes,
		NULL,
		NULL
	))
	{
		//WriteToConsole(stderr, "WSAIoctl() %d\n", WSAGetLastError());
		delete p_olp;
		return false;
	}

	p_olp->operation = IOCPContext::OP_ACCEPT;
	p_olp->client = preemptiveSocket;
	p_olp->listenSocket = m_hListenSocket.GetCopy();

	if (FALSE == lpfnAcceptEx(m_hListenSocket.GetCopy(), preemptiveSocket, pBuf, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &p_olp->dwBytes, &p_olp->ProviderOverlapped))
	{
		if (ERROR_IO_PENDING != WSAGetLastError())
		{
			//WriteToConsole(stderr, "AcceptEx() %d\n", WSAGetLastError());
			delete p_olp;
			return false;
		}
	}

	return true;
}

bool IOCP::IOCP::AcceptConnection(SOCKET clientListenSock)
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

DWORD IOCP::IOCP::WorkerThread(PVOID _pThreadInfo)
{
	if (NULL == _pThreadInfo)
	{
		fprintf(stderr, "WorkterThread() NULL PTR\n");
		return 0;
	}

	IOCPThreadInfo* pThreadInfo = (IOCPThreadInfo*)_pThreadInfo;
	auto pHandler = (std::shared_ptr<IOCP>)pThreadInfo->pIOCP;

	INT nBytesSent = 0;
	DWORD dwBytes = 0;
	DWORD dwBytesTransfered = 0;
	UINT_PTR nThreadNo = PtrToLong(pThreadInfo->pParam);
	OVERLAPPED* pOverlapped = NULL;
	IOCPContext* pContext = NULL;

	SOCKET targetSock = INVALID_SOCKET;

	// While not shutting down...
	while (WAIT_OBJECT_0 != WaitForSingleObject(pHandler->m_hShutdownEvent.get(), 0))
	{
		BOOL bReturn = GetQueuedCompletionStatus(pHandler->m_hIOCP.get(), &dwBytesTransfered, (PULONG_PTR)&pContext, &pOverlapped, INFINITE);

		// This catches errors and the destructor
		if (NULL == pContext)
		{
			break;
		}

		WSAOVERLAPPEDPLUS* pOverlappedPlus = NULL;
		pContext = (IOCPContext*)pContext;

		// Client disconnect check
		if (pContext->GetOpCode() != IOCPContext::OP_READ && pHandler->m_hListenSocket.GetCopy() != pContext->GetSocketCopy())
		{
			if ((FALSE == bReturn) || ((TRUE == bReturn) && (0 == dwBytesTransfered)))
			{
				// Client disconnected
				
				// TODO:
				/*pHandler->WriteToConsole(stderr, "Client Disconnected.\n");
				pHandler->RemoveFromClientListAndFreeMemory(pContext);*/
				
				continue;
			}
		}

		int nBytesRecv = 0;
		DWORD dwFlags = 0;
		WSAOVERLAPPED* p_ol = pContext->GetOVERLAPPEDPtr();

		switch (pContext->GetOpCode())
		{
		case IOCPContext::OP_ACCEPT:
			break;
		case IOCPContext::OP_LISTEN:
			// New connection
			pOverlappedPlus = CONTAINING_RECORD(pOverlapped, WSAOVERLAPPEDPLUS, ProviderOverlapped);
			setsockopt(pContext->GetSocketCopy(), SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&pHandler->m_hListenSocket, sizeof(pHandler->m_hListenSocket));
			pHandler->AcceptConnection(pOverlappedPlus->client);
			delete pOverlappedPlus;
			break;
		case IOCPContext::OP_READ:
			// Client is reading, server is writing
			pContext->IncrSentBytes(dwBytesTransfered);

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
					pHandler->RemoveFromClientListAndFreeMemory(pContext);
				}*/
			}
			// All data sent, post recv
			else
			{
				if (!pContext->ScheduleRecv())
				{
					/*pHandler->WriteToConsole(stderr, "Thread %d: WSARecv() %d\n", nThreadNo, WSAGetLastError());
					pHandler->RemoveFromClientListAndFreeMemory(pContext);*/
				}
				pContext->SetOpCode(IOCPContext::OP_WRITE);
			}
			break;
		case IOCPContext::OP_WRITE:
			// Client is writing, server is reading, send response (echo)
			char szBuffer[MAX_BUFFER_LEN];

			// Recving data is handled by accept thread
			if (!pContext->GetBuffer(szBuffer, sizeof(szBuffer)))
			{
				//pHandler->WriteToConsole(stderr, "CClientContext->GetBuffer()\n");
				break;
			}

			//pHandler->WriteToConsole(stdout, "Thread %d: Message: %s\n", nThreadNo, (char*)pkt.GetDataPointer());

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
				//pHandler->WriteToConsole(stderr, "Thread %d: WSASend() %d\n", nThreadNo, WSAGetLastError());
				//pHandler->RemoveFromClientListAndFreeMemory(pContext);
			}
			break;
		default:
			//pHandler->WriteToConsole(stderr, "Unhandled OP code.\n");
			break;
		}
	}
	return 0;
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