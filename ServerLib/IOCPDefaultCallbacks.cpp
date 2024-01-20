#include "IOCP.hpp"

#include <iostream>

bool IOCP::IOCP::DefaultMoreDataCb(const std::string_view& recvd, size_t& amountLeft)
{
	if(0 != recvd.back())
	{
		amountLeft = 1;
		return true;
	}
	else
	{
		amountLeft = 0;
		return false;
	}
}

bool IOCP::IOCP::DefaultProcessPacketCb(IOCPContext& context)
{
	std::string packet{};
	context.GetBufferContents(packet);

	unsigned char* buf = (unsigned char*)packet.c_str();
	int i, j;
	for(i = 0; i < packet.length(); i += 16)
	{
		printf("%06x: ", i);
		for(j = 0; j < 16; j++)
		{
			if(i + j < packet.length())
			{
				printf("%02x ", buf[i + j]);
			}
			else
			{
				printf("   ");
			}
		}

		printf(" ");
		for(j = 0; j < 16; j++)
		{
			if(i + j < packet.length())
			{
				printf("%c", isprint(buf[i + j]) ? buf[i + j] : '.');
			}
		}
		printf("\n");
	}

	context.BytesToSend = context.BytesRecvd;
	context.ScheduleSend();

	return true;
}