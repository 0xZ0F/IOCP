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

		// Will be used by the worker thread to decide what operation to perform.
		int m_nOpCode;

		// Total sent bytes
		int m_nSentBytesTotal;

		// Current number of sent bytes
		int m_nSentBytesCur;

		UniqueSocket m_hClientSocket;

		WSABUF m_wsaBuf;
		std::string m_wsaData;

	public:
		DWORD RecvdBytesTotal;

	public:
		IOCPContext();
		~IOCPContext();

		int GetOpCode() const;
		void SetOpCode(int n);

		int GetTotalBytes() const;
		void SetTotalSentBytes(int n);

		int GetSentBytes() const;
		void SetSentBytes(int n);
		void IncSentBytes(int n);

		void SetSocket(SOCKET sock);
		SOCKET GetSocketCopy() const;

		OVERLAPPED* GetOverlapped() const;

		bool ScheduleSend();

		/// <summary>
		/// Schedule a recv. If this is the first recieve, ensure the buffer is reset.
		/// </summary>
		/// <param name="stOffset">Offset into the existing buffer to write to.</param>
		/// <param name="stToRead">Amount of data to read, this is in addition to any existing data.</param>
		/// <returns>True on success, false otherwise.</returns>
		bool ScheduleRecv(size_t stOffset = 0, size_t stToRead = INITIAL_BUFFER_LEN);

		void SetBufferSize(size_t size);

		void GetBufferContents(std::string& strOut);

		void ResetBuffer();

		void Reset();
	};
}

