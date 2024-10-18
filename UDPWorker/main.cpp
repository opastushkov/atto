#include "Socket.h"
#include "../Common/Message.h"
#include "../Common/HashTable.h"

#include <thread>

LockFreeHashTable messageTable;

void UDPWorker(std::string ip, uint32_t port)
{
    Socket udpSock(Socket::Protocol::UDP);
    if(!udpSock.setNonBlocking())		{ return; }
    if(!udpSock.bind(ip.c_str(), port))	{ return; }

	std::cout << "[UDP WORKER] Started on " << ip << ":" << port << std::endl;

	Socket tcpSock(Socket::Protocol::TCP);
	if(!tcpSock.setNonBlocking())		{ return; }
	tcpSock.connect("127.0.0.1", 9002);

	while(true)
	{
		char buffer[sizeof(Message)];
		int bytesReceived = udpSock.recvfrom(buffer, sizeof(buffer));
		if(bytesReceived > 0)
		{
			Message* msg = new Message;
			memcpy(msg, buffer, sizeof(Message));

			std::cout << "[UDP WORKER][" << ip << ":" << port << "] "
				<< msg->messageId	<< " "
				<< msg->messageSize	<< " "
				<< msg->messageType	<< " "
				<< msg->messageData	<< std::endl;

			if(messageTable.find(msg->messageId)) { continue; }

			messageTable.insert(msg->messageId, msg);

			if(msg->messageData == 10) { tcpSock.send(buffer, sizeof(buffer)); }
		}
	}
}

int main()
{
	std::thread udpThread[2];
	for(int i = 0; i < 2; ++i) { udpThread[i] = std::thread(UDPWorker, "127.0.0.1", 9000 + i); }

	for(int i = 0; i < 2; ++i) { udpThread[i].join(); }

	return 0;
}