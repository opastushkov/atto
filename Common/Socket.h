#include <iostream>
#include <cstring>

class Socket
{
public:
    enum class Protocol { TCP, UDP };

    Socket(Protocol protocol);
    virtual ~Socket();

    bool        bind(const char* ip, int port);
    bool        connect(const char* ip, int port);
    bool        send(const char* data, int size);
    bool        recv(int clientSocket, char* buffer, int size);
    bool        recvfrom(char* buffer, int size);
    bool        listen();
    int         accept();
    void        close();
    bool        setNonBlocking();
    bool        isNonBlockingError();
    static int  pollSockets(int* sockets, int* triggeredSockets);
    int         getSocket() const { return socket; }

private:
    Protocol            protocol;
    int                 socket;
    static const int    INVALID_SOCK = -1;
    static const int    SOCK_ERROR = -1;
};
