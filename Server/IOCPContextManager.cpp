#include "IOCPContextManager.hpp"

IOCP::IOCPContextManager::IOCPContextManager()
{
	InitializeCriticalSection(&m_cs);
}

IOCP::IOCPContextManager::~IOCPContextManager()
{
	DeleteCriticalSection(&m_cs);
}

std::weak_ptr<IOCP::IOCPContext> IOCP::IOCPContextManager::AddContext(std::shared_ptr<IOCPContext> pContext)
{
	m_vContexts.push_back(pContext);

	return pContext;
}

bool IOCP::IOCPContextManager::RemoveContext(std::shared_ptr<IOCPContext> pContext)
{
	EnterCriticalSection(&m_cs);

	std::vector<std::shared_ptr<IOCPContext>>::iterator iterClientContext;

	for (iterClientContext = m_vContexts.begin(); iterClientContext != m_vContexts.end(); iterClientContext++)
	{
		if (pContext == *iterClientContext)
		{
			m_vContexts.erase(iterClientContext);
			break;
		}
	}

	LeaveCriticalSection(&m_cs);

	return TRUE;
}