#pragma once

#include <string>
#include "UniqueSocket.hpp"

// Buffer Length 
#define INITIAL_BUFFER_LEN 256

namespace IOCP
{
	class IOCPContext
	{
	public:
		enum OP_CODES
		{
			OP_READ = 0,
			OP_WRITE,
			OP_ACCEPT,
			OP_LISTEN,
		};
	private:
		OVERLAPPED* m_pOverlapped;

		UniqueSocket m_hClientSocket;

		WSABUF m_wsaBuf;
		std::string m_wsaData;

	public:
		// Total number of bytes to be sent.
		DWORD BytesToSend;
		// Number of bytes which have been sent so far.
		DWORD BytesSent;

		// Number of bytes to receive. Only set for partial reads.
		DWORD BytesToRecv;
		// Number of bytes received so far.
		DWORD BytesRecvd;

		// Current opcode (Ex. IOCPContext::OP_READ or IOCPContext::OP_WRITE)
		DWORD OpCode;

	public:
		IOCPContext();
		~IOCPContext();

		void SetSocket(SOCKET sock);
		SOCKET GetSocketCopy() const;

		OVERLAPPED* GetOverlapped() const;

		bool ScheduleSend(ULONG ulOffset = 0);

		/// <summary>
		/// Schedule a recv. If this is the first recieve, ensure the buffer is reset.
		/// </summary>
		/// <param name="stOffset">Offset into the existing buffer to write to.</param>
		/// <param name="stToRead">Amount of data to read, this is in addition to any existing data.</param>
		/// <returns>True on success, false otherwise.</returns>
		bool ScheduleRecv(ULONG ulOffset = 0, ULONG ulToRead = INITIAL_BUFFER_LEN);

		void SetBufferSize(size_t size);

		void GetBufferContents(std::string& strOut);

		void ResetBuffer();

		void Reset();
	};
}

