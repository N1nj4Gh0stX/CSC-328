/*************************************************************/
/* author: []                                                 */
/* filename: socket.h                                        */
/* purpose: this header file declares the mysock class,      */
/*          which encapsulates socket functionality for      */
/*          client-server communication, including methods   */
/*          for connecting, sending, receiving, and closing  */
/*          connections.                                     */
/*************************************************************/

#ifndef SOCKET_H
#define SOCKET_H

#include <string>

/*************************************************************/
/* class: mysock                                             */
/* purpose: a class that provides socket functionality for   */
/*          client-server communication. it includes methods */
/*          to connect to a server, send and receive data,   */
/*          bind to a port, listen for incoming connections, */
/*          and close the socket.                            */
/*************************************************************/

class mysock {
  public:
    /*************************************************************/
    /* function: mysock                                         */
    /* purpose: default constructor for mysock object.          */
    /*          initializes the socket object and sets up an    */
    /*          unconnected socket descriptor.                  */
    /*************************************************************/
    mysock();

    /*************************************************************/
    /* function: mysock                                         */
    /* purpose: constructor for mysock with a specified socket  */
    /*          file descriptor.                               */
    /* parameters:                                              */
    /*    - fd: the socket file descriptor to initialize the    */
    /*          socket object with.                             */
    /*************************************************************/
    mysock(int fd);

    /*************************************************************/
    /* function: connect                                        */
    /* purpose: connects the socket to a remote server.         */
    /* parameters:                                              */
    /*    - hostname: the remote server's hostname or ip        */
    /*                 address.                                 */
    /*    - port: the port number to connect to.                */
    /*************************************************************/
    void connect(const std::string &hostname, const std::string &port);

    /*************************************************************/
    /* function: close                                          */
    /* purpose: closes the socket.                              */
    /*          terminates the connection by closing the socket.*/
    /*************************************************************/
    void close();

    /*************************************************************/
    /* function: clientrecv                                     */
    /* purpose: receives data from the socket.                  */
    /* parameters:                                              */
    /*    - buffer: the buffer to store the received data.      */
    /*    - size: the size of the buffer.                       */
    /* return: the number of bytes received, or a negative      */
    /*         value on error.                                  */
    /*************************************************************/
    int clientrecv(char *buffer, size_t size);

    /*************************************************************/
    /* function: clientsend                                     */
    /* purpose: sends a message to the connected server.        */
    /* parameters:                                              */
    /*    - message: the message to be sent.                    */
    /* return: the number of bytes sent, or a negative value    */
    /*         on error.                                        */
    /*************************************************************/
    int clientsend(const std::string &message); // ensure this returns int

    /*************************************************************/
    /* function: bind                                           */
    /* purpose: binds the socket to a specified port.           */
    /* parameters:                                              */
    /*    - port: the port number to bind the socket to.        */
    /*************************************************************/
    void bind(const std::string &port);

    /*************************************************************/
    /* function: listen                                         */
    /* purpose: listens for incoming connections on the socket. */
    /* parameters:                                              */
    /*    - backlog: the maximum length of the queue of pending */
    /*                connections.                               */
    /*************************************************************/
    void listen(int backlog);

    /*************************************************************/
    /* function: accept                                         */
    /* purpose: accepts an incoming connection.                 */
    /* return: a mysock object representing the accepted         */
    /*         connection.                                      */
    /*************************************************************/
    mysock accept();

  private:
    int fd; //socket file descriptor representing the socket.
};

#endif
