#include "IOCPContext.hpp"

IOCP::IOCPContext::IOCPContext()
{
	m_pOverlapped = new WSAOVERLAPPED;
	m_wsaData.resize(MAX_BUFFER_LEN);

	ZeroMemory(m_pOverlapped, sizeof(OVERLAPPED));

	m_wsaBuf.buf = const_cast<char*>(m_wsaData.data());
	m_wsaBuf.len = (ULONG)m_wsaData.length();

	m_nOpCode = 0;
	m_nTotalBytes = 0;
	m_nSentBytes = 0;
}

IOCP::IOCPContext::~IOCPContext()
{
	delete m_pOverlapped;
}

void IOCP::IOCPContext::SetOpCode(int n)
{
	m_nOpCode = n;
}

int IOCP::IOCPContext::GetOpCode() const
{
	return m_nOpCode;
}

void IOCP::IOCPContext::SetTotalBytes(int n)
{
	m_nTotalBytes = n;
}

int IOCP::IOCPContext::GetTotalBytes() const
{
	return m_nTotalBytes;
}

void IOCP::IOCPContext::SetSentBytes(int n)
{
	m_nSentBytes = n;
}

void IOCP::IOCPContext::IncrSentBytes(int n)
{
	m_nSentBytes += n;
}

void IOCP::IOCPContext::SetSocket(SOCKET sock)
{
	m_hClientSocket.SetSocket(sock);
}

SOCKET IOCP::IOCPContext::GetSocketCopy() const
{
	return m_hClientSocket.GetCopy();
}

int IOCP::IOCPContext::GetSentBytes() const
{
	return m_nSentBytes;
}

OVERLAPPED* IOCP::IOCPContext::GetOVERLAPPEDPtr() const
{
	return m_pOverlapped;
}

bool IOCP::IOCPContext::ScheduleSend()
{
	DWORD dwRet = 0;
	DWORD dwSent = 0;
	dwRet = WSASend(m_hClientSocket.GetCopy(), &m_wsaBuf, 1, &dwSent, 0, m_pOverlapped, NULL);

	if ((SOCKET_ERROR == dwRet) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		return false;
	}

	return true;
}

bool IOCP::IOCPContext::ScheduleRecv()
{	
	//ResetWSABUF();

	DWORD dwFlags = 0;
	DWORD dwBytes = 0;

	// Post a recv for this client
	if (SOCKET_ERROR == WSARecv(GetSocketCopy(), &m_wsaBuf, 1, &dwBytes, &dwFlags, m_pOverlapped, NULL))
	{
		if (WSA_IO_PENDING != WSAGetLastError())
		{
			return false;
		}
	}

	return true;
}

/// <summary>
/// Copy the provided buffer into the private buffer.
/// </summary>
/// <param name="szBuffer">Buffer to copy into the private buffer.</param>
/// <param name="len">Size of the provided buffer, not to be larger than the private buffer.</param>
/// <returns>TRUE on success, FALSE on failure.</returns>
BOOL IOCP::IOCPContext::SetBuffer(PCSTR szBuffer, size_t len)
{
	if (NULL == szBuffer || len < 1 || len > sizeof(m_wsaBuf.len))
	{
		return false;
	}

	memcpy_s(m_wsaBuf.buf, m_wsaBuf.len, szBuffer, len);
	return true;
}

/// <summary>
/// Copy from the private buffer into the provided buffer.
/// </summary>
/// <param name="szBuffer">Buffer to copy into.</param>
/// <param name="len">Size of the buffer, not to be larger than the private buffer.</param>
/// <returns>TRUE on success, FALSE on failure.</returns>
BOOL IOCP::IOCPContext::GetBuffer(char* szBuffer, size_t len) const
{
	if (NULL == szBuffer || len < 1 || len > sizeof(m_wsaBuf.len))
	{
		return FALSE;
	}

	memcpy_s(szBuffer, len, m_wsaBuf.buf, sizeof(m_wsaBuf.len));
	return TRUE;
}

/// <summary>
/// Zero the private buffer.
/// </summary>
void IOCP::IOCPContext::ZeroBuffer()
{
	ZeroMemory(m_wsaBuf.buf, m_wsaBuf.len);
}
