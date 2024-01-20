#include "UniqueSocket.hpp"

IOCP::UniqueSocket::UniqueSocket() : m_hSocket(NULL)
{}

IOCP::UniqueSocket::UniqueSocket(SOCKET&& sock) : m_hSocket(sock)
{}

IOCP::UniqueSocket::~UniqueSocket()
{
	closesocket(m_hSocket);
}

bool IOCP::UniqueSocket::CreateSocketW(
	_In_ int af,
	_In_ int type,
	_In_ int protocol,
	_In_opt_ LPWSAPROTOCOL_INFOW lpProtocolInfo,
	_In_ GROUP g,
	_In_ DWORD dwFlags)
{
	if (m_hSocket && INVALID_SOCKET != m_hSocket)
	{
		if (SOCKET_ERROR == closesocket(m_hSocket))
		{
			return false;
		}
	}

	m_hSocket = WSASocketW(af, type, protocol, lpProtocolInfo, g, dwFlags);
	if (INVALID_SOCKET == m_hSocket)
	{
		return false;
	}

	return true;
}

SOCKET IOCP::UniqueSocket::GetSocket() const
{
	return m_hSocket;
}

void IOCP::UniqueSocket::SetSocket(SOCKET sock)
{
	m_hSocket = sock;
}
