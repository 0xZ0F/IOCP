#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>

/// <summary>
/// This structure is used to pass additional information through AcceptEx().
/// When GetQueuedCompletionStatus() returns, it will return a pointer
/// to the WSAOVERLAPPED structure created for and used by AcceptEx().
/// By passing our custom structure, CONTAINING_RECORD can be used
/// to obtain the base of this custom data structure.
/// </summary>
typedef struct _WSAOVERLAPPEDPLUS
{
    WSAOVERLAPPED wsaOverlapped; // 'WSAOVERLAPPED' has to be the first member.
    DWORD dwBytesReceived;
    SOCKET hClientSocket; // Use this to pass preemptive accept socket.
    SOCKET hListenSocket; // Use this to pass the listenSocket.
    void* pAcceptBuf;
    size_t sAcceptBufLen;
} WSAOVERLAPPEDPLUS, * LPWSAOVERLAPPEDPLUS;

WSAOVERLAPPEDPLUS* AllocWSAOverlappedPlus(size_t sAcceptBufLen = 0)
{
    auto pOLP = new WSAOVERLAPPEDPLUS;
    ZeroMemory(pOLP, sizeof(*pOLP));

    pOLP->sAcceptBufLen = sAcceptBufLen;
    if (sAcceptBufLen)
    {
        pOLP->pAcceptBuf = new char[pOLP->sAcceptBufLen];
    }

    return pOLP;
}

bool FreeWSAOverlappedPlus(WSAOVERLAPPEDPLUS** pOLP)
{
    if (NULL == pOLP || NULL == *pOLP)
    {
        return false;
    }

    delete[] (*pOLP)->pAcceptBuf;
    
    delete *pOLP;
    *pOLP = NULL;

    return true;
}