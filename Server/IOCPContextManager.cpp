#include "IOCPContextManager.hpp"

IOCP::IOCPContextManager::IOCPContextManager()
{
	InitializeCriticalSection(&m_cs);
}

IOCP::IOCPContextManager::~IOCPContextManager()
{
	DeleteCriticalSection(&m_cs);
}

std::shared_ptr<IOCP::IOCPContext> IOCP::IOCPContextManager::CreateContext()
{
	auto pContext = std::make_shared<IOCPContext>();
	AddContext(pContext);
	return pContext;
}

void IOCP::IOCPContextManager::AddContext(std::shared_ptr<IOCPContext> pContext)
{
	m_vContexts.push_back(pContext);
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

bool IOCP::IOCPContextManager::RemoveContext(IOCPContext* pContext)
{
	EnterCriticalSection(&m_cs);

	auto found = std::find_if(m_vContexts.begin(), m_vContexts.end(),
		[&pContext](const std::shared_ptr<IOCPContext> pToFind) {
			pToFind.get() == pContext;
		}
	);

	m_vContexts.erase(found);

	LeaveCriticalSection(&m_cs);

	return TRUE;
}