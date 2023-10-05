#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>

typedef struct _WSAOVERLAPPEDPLUS
{
    WSAOVERLAPPED ProviderOverlapped; // 'WSAOVERLAPPED' has to be the first member.
    INT operation;
    DWORD dwBytes;
    SOCKET client; // Use this to pass preemptive accept socket.
    SOCKET listenSocket; // Use this to pass the listenSocket.
} WSAOVERLAPPEDPLUS, * LPWSAOVERLAPPEDPLUS;