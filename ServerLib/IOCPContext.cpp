#include "IOCPContext.hpp"

IOCP::IOCPContext::IOCPContext()
{
	m_pOverlapped = new WSAOVERLAPPED;
	m_wsaData.resize(INITIAL_BUFFER_LEN);

	ZeroMemory(m_pOverlapped, sizeof(OVERLAPPED));

	ResetBuffer();

	OpCode = 0;
	BytesToSend = 0;
	BytesSent = 0;
	BytesRecvd = 0;
}

IOCP::IOCPContext::~IOCPContext()
{
	delete m_pOverlapped;
}

void IOCP::IOCPContext::SetSocket(SOCKET sock)
{
	m_hClientSocket.SetSocket(sock);
}

SOCKET IOCP::IOCPContext::GetSocketCopy() const
{
	return m_hClientSocket.GetSocket();
}

OVERLAPPED* IOCP::IOCPContext::GetOverlapped() const
{
	return m_pOverlapped;
}

bool IOCP::IOCPContext::ScheduleSend(ULONG ulOffset)
{
	DWORD dwRet = 0;
	DWORD dwSent = 0;

	if(ulOffset >= m_wsaBuf.len)
	{
		return false;
	}

	m_wsaBuf.buf = m_wsaData.data() + ulOffset;
	m_wsaBuf.len -= ulOffset;

	dwRet = WSASend(m_hClientSocket.GetSocket(), &m_wsaBuf, 1, &dwSent, 0, m_pOverlapped, NULL);
	if((SOCKET_ERROR == dwRet) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		return false;
	}

	return true;
}

bool IOCP::IOCPContext::ScheduleRecv(ULONG ulOffset, ULONG ulToRead)
{
	//ResetWSABUF();

	DWORD dwFlags = 0;
	DWORD dwBytes = 0;

	size_t finalIndex = ulOffset + ulToRead;

	if(finalIndex > m_wsaBuf.len)
	{
		SetBufferSize(finalIndex);
	}
	m_wsaBuf.buf = m_wsaData.data() + ulOffset;
	m_wsaBuf.len = ulToRead;

	// Post a recv for this client
	if(SOCKET_ERROR == WSARecv(GetSocketCopy(), &m_wsaBuf, 1, &dwBytes, &dwFlags, m_pOverlapped, NULL))
	{
		if(WSA_IO_PENDING != WSAGetLastError())
		{
			return false;
		}
	}

	return true;
}

void IOCP::IOCPContext::SetBufferSize(size_t size)
{
	m_wsaData.resize(size);
	m_wsaBuf.len = (ULONG)m_wsaData.length();
}

void IOCP::IOCPContext::GetBufferContents(std::string& strOut)
{
	strOut.assign(m_wsaData.begin(), m_wsaData.end());
}

void IOCP::IOCPContext::ResetBuffer()
{
	m_wsaData.clear();
	m_wsaBuf.buf = m_wsaData.data();
	m_wsaBuf.len = (ULONG)m_wsaData.length();
}

void IOCP::IOCPContext::Reset()
{
	ResetBuffer();
	BytesSent = 0;
	BytesToSend = 0;
	BytesRecvd = 0;
}
