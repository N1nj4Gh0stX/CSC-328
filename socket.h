/*************************************************************/
/* Author: Dr. Schwesinger edited by []
/* Filename: socket.h
/* Purpose: This header file declares socket functions.
/************************************************************/
#ifndef SOCKET_H
#define SOCKET_H

using namespace std;

/*************************************************************************/
/* class name: mysock
/*	A class to handle socket operations.
/* 
/* This class manages the server socket, such as setting it up, 
/* sending, receiving data, binding, connecting and listening to for client socket 
/* and closing the socket when done.
/*************************************************************************/
class mysock {
  public:

	mysock();
	mysock(int fd);
	/*************************************************************************/
	/* Function name: close                                                 
	/* Description: closes the socket after use                             
	/* Parameters: none                       
	/* Return Value: Void                                                    
	/*************************************************************************/
	void close();  
	/*************************************************************************/
	/* Function name: connect                                                
	/* Description: connects socket to remote host                            
	/* Parameters: host - the filesname
	/*	       port - the port number
	/* Return Value: Void                                                    
	/**********************************************************************/
	void connect(string host, string port);
	/*************************************************************************/
	/* Function name: bind                                                
	/* Description: binds socket to port                          
	/* Parameters:  port - the port number
	/* Return Value: Void                                                    
	/*************************************************************************/
  void bind(string port);
  /*************************************************************************/
	/* Function name: listen                                                
	/* Description: connect queue                          
	/* Parameters: backlog - amount of connections allowed in queue
	/* Return Value: Void                                                    
	/*************************************************************************/
	void listen(int backlog);
	/*************************************************************************/
	/* Function name: accept                                                
	/* Description: Accept incoming client connections
	/* Parameters:  none
	/* Return Value: Void                                                    
	/*************************************************************************/	
  mysock accept();

	int clientrecv();
	int clientsend(string message);
		
	int serverrecv();
	int serversend(string message);
	
};
#endif
