#include <iostream>

#include "DebugPrint.h"
#include "IOCP.hpp"

int main(int argc, char** argv)
{
	DEBUG_PRINT("Debug On\n");

	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <PORT>\n", argv[0]);
		return 0;
	}

	IOCP::IOCP iocp;
	if (!iocp.Init())
	{
		DEBUG_PRINT("iocp.Init() WSA_GLE: %d\n", WSAGetLastError());
		return -1;
	}

	u_short usPort = atoi(argv[1]);
	if (!iocp.Begin(usPort))
	{
		DEBUG_PRINT("iocp.Init() WSA_GLE: %d\n", WSAGetLastError());
		return -1;
	}

	printf("...\n");
	std::getchar();

	return 0;
}