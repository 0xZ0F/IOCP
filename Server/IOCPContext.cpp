#include "IOCPContext.hpp"

IOCP::IOCPContext::IOCPContext()
{
	m_pOverlapped = new WSAOVERLAPPED;
	m_wsaData.resize(INITIAL_BUFFER_LEN);

	ZeroMemory(m_pOverlapped, sizeof(OVERLAPPED));

	ResetBuffer();

	m_nOpCode = 0;
	m_nSentBytesTotal = 0;
	m_nSentBytesCur = 0;
	RecvdBytesTotal = 0;
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

void IOCP::IOCPContext::SetTotalSentBytes(int n)
{
	m_nSentBytesTotal = n;
}

int IOCP::IOCPContext::GetTotalBytes() const
{
	return m_nSentBytesTotal;
}

void IOCP::IOCPContext::SetSentBytes(int n)
{
	m_nSentBytesCur = n;
}

void IOCP::IOCPContext::IncSentBytes(int n)
{
	m_nSentBytesCur += n;
}

void IOCP::IOCPContext::SetSocket(SOCKET sock)
{
	m_hClientSocket.SetSocket(sock);
}

SOCKET IOCP::IOCPContext::GetSocketCopy() const
{
	return m_hClientSocket.GetSocket();
}

int IOCP::IOCPContext::GetSentBytes() const
{
	return m_nSentBytesCur;
}

OVERLAPPED* IOCP::IOCPContext::GetOverlapped() const
{
	return m_pOverlapped;
}

bool IOCP::IOCPContext::ScheduleSend()
{
	DWORD dwRet = 0;
	DWORD dwSent = 0;
	dwRet = WSASend(m_hClientSocket.GetSocket(), &m_wsaBuf, 1, &dwSent, 0, m_pOverlapped, NULL);

	if ((SOCKET_ERROR == dwRet) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		return false;
	}

	return true;
}

bool IOCP::IOCPContext::ScheduleRecv(size_t stOffset, size_t stToRead)
{	
	//ResetWSABUF();

	DWORD dwFlags = 0;
	DWORD dwBytes = 0;

	SetBufferSize(stOffset + stToRead);
	m_wsaBuf.buf += stOffset;

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
	m_nSentBytesCur = 0;
	m_nSentBytesTotal = 0;
	RecvdBytesTotal = 0;
}
