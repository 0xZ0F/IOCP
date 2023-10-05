#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>

namespace IOCP
{
	class UniqueSocket
	{
	protected:
		SOCKET m_hSocket;

	public:
		UniqueSocket();
		UniqueSocket(SOCKET&& sock);
		UniqueSocket(const UniqueSocket&) = delete;
		UniqueSocket(UniqueSocket&&) = delete;
		void operator=(const UniqueSocket&) = delete;
		void operator=(UniqueSocket&&) = delete;
		~UniqueSocket();

		/// <summary>
		/// Wrapper for WSASocketW();
		/// </summary>
		bool CreateSocketW(
			_In_ int af,
			_In_ int type,
			_In_ int protocol,
			_In_opt_ LPWSAPROTOCOL_INFOW lpProtocolInfo,
			_In_ GROUP g,
			_In_ DWORD dwFlags);

		/// <summary>
		/// Get a copy of the socket handle.
		/// </summary>
		/// <returns>Returns a socket handle.</returns>
		SOCKET GetCopy() const;

		void SetSocket(SOCKET sock);
	};
}

