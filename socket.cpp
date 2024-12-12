/*************************************************************/
/* author: []                                                 */
/* filename: socket.cpp                                       */
/* purpose: this source file implements the mysock class,    */
/*          which provides socket functionality for          */
/*          client-server communication. it defines methods   */
/*          for connecting to a server, sending and receiving*/
/*          data, binding to a port, listening for incoming  */
/*          connections, and closing the socket.             */
/*          the methods utilize system calls such as socket, */
/*          connect, send, and recv to manage communication. */
/*************************************************************/
#include "socket.h"
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>

mysock::mysock() {
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        throw std::runtime_error("Failed to create socket");
    }
}

mysock::mysock(int fd) : fd(fd) {}

void mysock::close() {
    if (::close(fd) == -1) {
        throw std::runtime_error("Failed to close socket");
    }
}

void mysock::connect(const std::string &hostname, const std::string &port) {
    struct addrinfo hints, *res;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;        // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP

    if (getaddrinfo(hostname.c_str(), port.c_str(), &hints, &res) != 0) {
        throw std::runtime_error("Failed to resolve hostname");
    }

    if (::connect(fd, res->ai_addr, res->ai_addrlen) == -1) {
        freeaddrinfo(res);
        throw std::runtime_error("Connection failed");
    }

    freeaddrinfo(res);
}

void mysock::bind(const std::string &port) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(nullptr, port.c_str(), &hints, &res) != 0) {
        throw std::runtime_error("Failed to get address info");
    }

    if (::bind(fd, res->ai_addr, res->ai_addrlen) == -1) {
        freeaddrinfo(res);
        throw std::runtime_error("Failed to bind socket");
    }

    freeaddrinfo(res);
}


void mysock::listen(int backlog) {
    if (::listen(fd, backlog) == -1) {
        throw std::runtime_error("Failed to listen on socket");
    }
}

mysock mysock::accept() {
    int client_fd = ::accept(fd, nullptr, nullptr);
    if (client_fd == -1) {
        throw std::runtime_error("Failed to accept connection");
    }
    return mysock(client_fd);
}

int mysock::clientsend(const std::string &message) {
    if (send(fd, message.c_str(), message.size(), 0) == -1) {
        perror("send");
        return -1; // Indicate failure
    }
    return 0; // Indicate success
}

int mysock::clientrecv(char *buffer, size_t size) {
    int bytes = recv(fd, buffer, size, 0);
    if (bytes == -1) {
        throw std::runtime_error("Failed to receive message");
    }
    return bytes;
}


