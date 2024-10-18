#pragme once

#include "Socket.h"

#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#endif

Socket::Socket(Protocol protocol) : protocol(protocol), socket(INVALID_SOCK)
{
#if defined(_WIN32) || defined(_WIN64)
    WSADATA wsaData;
    if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed\n";
        return;
    }
#endif

    if(protocol == Protocol::TCP)   { socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); }
    else                            { socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); }

    if(socket == INVALID_SOCK)      { std::cerr << "Failed to create socket\n"; }
}

Socket::~Socket()
{
    close();
#if defined(_WIN32) || defined(_WIN64)
    WSACleanup();
#endif
}

bool Socket::bind(const char* ip, int port)
{
    sockaddr_in addr{};
    addr.sin_family         = AF_INET;
    addr.sin_port           = htons(port);
    addr.sin_addr.s_addr    = inet_addr(ip);

    if(::bind(socket, (struct sockaddr*)&addr, sizeof(addr)) == SOCK_ERROR)
    {
        std::cerr << "Bind failed\n";
        return false;
    }

    return true;
}

bool Socket::connect(const char* ip, int port)
{
    sockaddr_in addr{};
    addr.sin_family         = AF_INET;
    addr.sin_port           = htons(port);
    addr.sin_addr.s_addr    = inet_addr(ip);

    if(::connect(socket, (struct sockaddr*)&addr, sizeof(addr)) == SOCK_ERROR)
    {
        if(isNonBlockingError()) { return true; }
        std::cerr << "Connect failed\n";
        return false;
    }

    return true;
}

bool Socket::send(const char* data, int size)
{
    int result = ::send(socket, data, size, 0);
    if(result == SOCK_ERROR)
    {
        if(isNonBlockingError()) { return true; }
        std::cerr << "Send failed\n";
        return false;
    }

    return true;
}

bool Socket::recv(int clientSocket, char* buffer, int size)
{
    int result = ::recv(clientSocket, buffer, size, 0);
    if(result == SOCK_ERROR)
    {
        if(isNonBlockingError()) { return true; }
        std::cerr << "Recv failed\n";
        return false;
    }
    else if(result == 0)
    {
        std::cerr << "Connection closed by peer\n";
        return false;
    }

    return true;
}

bool Socket::recvfrom(char* buffer, int size)
{
    int result = ::recvfrom(socket, buffer, size, 0, nullptr, nullptr);
    if(result == SOCK_ERROR)
    {
        if(!isNonBlockingError()) { return true; }
        return false;
    }
    else if(result == 0)
    {
        std::cerr << "Connection closed by peer\n";
        return false;
    }

    buffer[result] = '\0';

    return true;
}

bool Socket::listen()
{
    if(::listen(socket, SOMAXCONN) == SOCK_ERROR)
    {
        std::cerr << "Listen failed\n";
        return false;
    }

    return true;
}

int Socket::accept()
{
    int clientSocket = ::accept(socket, nullptr, nullptr);
    if(clientSocket == SOCK_ERROR) { return INVALID_SOCK; }

    return clientSocket;
}

void Socket::close()
{
    if(socket != INVALID_SOCK)
    {
#if defined(_WIN32) || defined(_WIN64)
        closesocket(socket);
#else
        ::close(socket);
#endif
        socket = INVALID_SOCK;
    }
}

bool Socket::setNonBlocking() {
#if defined(_WIN32) || defined(_WIN64)
    u_long mode = 1;
    if(ioctlsocket(socket, FIONBIO, &mode) != 0)
    {
        std::cerr << "Failed to set non-blocking mode\n";
        return false;
    }
#else
    int flags = fcntl(socket_, F_GETFL, 0);
    if(flags == -1)
    {
        std::cerr << "Failed to get flags\n";
        return false;
    }
    if(fcntl(socket_, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        std::cerr << "Failed to set non-blocking mode\n";
        return false;
    }
#endif

    return true;
}

bool Socket::isNonBlockingError()
{
#if defined(_WIN32) || defined(_WIN64)
    return WSAGetLastError() == WSAEWOULDBLOCK;
#else
    return errno == EWOULDBLOCK || errno == EAGAIN;
#endif
}

int Socket::pollSockets(int* sockets, int* triggeredSockets)
{
    struct  pollfd fds[3];
    int     triggeredCount = 0;

    for(int i = 0; i < 3; ++i)
    {
        fds[i].fd = sockets[i];
        fds[i].events = POLLIN;
    }

#ifdef _WIN32
    int result = WSAPoll(fds, 3, -1);
#else
    int result = poll(fds, 3, -1);
#endif

    if(result < 0) { throw std::runtime_error("Poll failed."); }

    for(int i = 0; i < 3; ++i)
    {
        if(fds[i].revents & POLLIN)
        {
            triggeredSockets[triggeredCount++] = i;
        }
    }

    return triggeredCount;
}