#include <iostream>

#include "ServerLib/IOCP.hpp"

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <PORT>\n", argv[0]);
		return 0;
	}

	IOCP::IOCP iocp;

	u_short usPort = atoi(argv[1]);
	if (!iocp.Begin(usPort))
	{
		fprintf(stderr, "iocp.Begin()\n");
		return -1;
	}

	std::getchar();

	return 0;
}