#include <iostream>

#include "Logger/Logger.hpp"
#include "ServerLib/DebugPrint.h"
#include "ServerLib/IOCP.hpp"

#ifndef NDEBUG
Logger::Logger _g_logger;
#endif

int main(int argc, char** argv)
{
	DEBUG_PRINT("Debug On\n");

	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <PORT>\n", argv[0]);
		return 0;
	}

	IOCP::IOCP iocp;

	u_short usPort = atoi(argv[1]);
	if (!iocp.Begin(usPort))
	{
		DEBUG_PRINT("iocp.Init() WSA_GLE: %d\n", WSAGetLastError());
		return -1;
	}

	std::getchar();

	return 0;
}