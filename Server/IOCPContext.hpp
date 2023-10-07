#pragma once

#include <string>
#include "UniqueSocket.hpp"

// Buffer Length 
#define MAX_BUFFER_LEN 256

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
		int m_nTotalBytes;
		
		// Current number of sent bytes
		int m_nSentBytes;
		
		UniqueSocket m_hClientSocket;
		
		WSABUF m_wsaBuf;
		std::string m_wsaData;

	public:
		IOCPContext();
		~IOCPContext();

		int GetOpCode() const;
		void SetOpCode(int n);

		int GetTotalBytes() const;
		void SetTotalBytes(int n);

		int GetSentBytes() const;
		void SetSentBytes(int n);
		void IncSentBytes(int n);

		void SetSocket(SOCKET sock);
		SOCKET GetSocketCopy() const;

		OVERLAPPED* GetOverlapped() const;

		bool ScheduleSend();

		bool ScheduleRecv();

		/// <summary>
		/// Copy the provided buffer into the private buffer.
		/// </summary>
		/// <param name="szBuffer">Buffer to copy into the private buffer.</param>
		/// <param name="len">Size of the provided buffer, not to be larger than the private buffer.</param>
		/// <returns>TRUE on success, FALSE on failure.</returns>
		BOOL SetBuffer(PCSTR szBuffer, size_t len);

		/// <summary>
		/// Copy from the private buffer into the provided buffer.
		/// </summary>
		/// <param name="szBuffer">Buffer to copy into.</param>
		/// <param name="len">Size of the buffer, not to be larger than the private buffer.</param>
		/// <returns>TRUE on success, FALSE on failure.</returns>
		BOOL GetBuffer(char* szBuffer, size_t len) const;

		/// <summary>
		/// Zero the private buffer.
		/// </summary>
		void ZeroBuffer();
	};
}

