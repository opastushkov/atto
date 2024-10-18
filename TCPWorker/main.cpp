#include "Socket.h"
#include "HashTable.h"

int main()
{
    Socket tcpSock(Socket::Protocol::TCP);
    if(!tcpSock.setNonBlocking())           { return 1; }
    if(!tcpSock.bind("127.0.0.1", 9002))    { return 1; }
    if(!tcpSock.listen())                   { return 1; }

    std::cout << "[TCP WORKER] Started on 127.0.0.1:9001" << std::endl;

    int sockets[3] = { tcpSock.getSocket(), -1, -1 };
    while(true)
    {
        int triggeredSockets[3] = { -1, -1, -1 };
        uint32_t result = Socket::pollSockets(sockets, triggeredSockets);
        for(int i = 0; i < 3; ++i)
        {
            int index = triggeredSockets[i];
            if(index == 0)
            {
                int clientSocket = tcpSock.accept();
                std::cout << "[TCP WORKER] Client accepted" << std::endl;
                if(clientSocket == -1) { continue; }

                for(int j = 1; j < 3; ++j)
                {
                    if(sockets[j] == -1)
                    {
                        sockets[j] = clientSocket;
                        break;
                    }
                }
            }
            else if(index > 0)
            {
                char    buffer[sizeof(Message)];
                int     bytesReceived = tcpSock.recv(sockets[index], buffer, sizeof(buffer));
                if(bytesReceived > 0)
                {
                    Message* msg = new Message;
                    memcpy(msg, buffer, sizeof(Message));
                    std::cout << "[TCP WORKER][127.0.0.1:9002] "
                        << msg->messageId << " "
                        << msg->messageSize << " "
                        << msg->messageType << " "
                        << msg->messageData << std::endl;
                }
                else
                {
                    tcpSock.close();
                    sockets[index] = -1;
                }
            }
        }
    }

    return 0;
}