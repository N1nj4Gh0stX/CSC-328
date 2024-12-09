#ifndef SOCKET_H
#define SOCKET_H

#include <string>

class mysock {
  public:
    mysock();
    mysock(int fd);
    void connect(const std::string &hostname, const std::string &port);
    void close();
    int clientrecv(char *buffer, size_t size);
    int clientsend(const std::string &message); // Ensure this returns int
    void bind(const std::string &port);
    void listen(int backlog);
    mysock accept();
  private:
    int fd;
};

#endif
