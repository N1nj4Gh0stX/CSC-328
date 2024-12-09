#ifndef SOCKET_H
#define SOCKET_H

#include <string.h>

using namespace std;

class mysocket {

    public:
    int fd;
    // constructor
    mysocket();
    void close();
    void connect(string host, string port);
    void sendall(string message);
    string recvall();
    // int really_recv(int s, char* buf, int len);
    // int really_send(int s, char *buf, int len);
    
};

#endif // SOCKET_H
