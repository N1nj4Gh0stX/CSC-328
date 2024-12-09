//liz socket.cpp
#include <iostream>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sstream>

#include "socket.h"

using namespace std;

mysocket::mysocket(){
	if ((this->fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        stringstream s;
        s << "socket: " << strerror(errno) << endl;
        throw runtime_error(s.str());
}
}

void mysocket::close(){
	::close(fd);
}

void mysocket::connect(string host, string port){
	struct addrinfo hints, *res;
	int retval;
	
	memset(&hints, 0 , sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	
	if ((retval = getaddrinfo(host.c_str(), port.c_str(), &hints, &res)) != 0){
		stringstream s;
		s<< "getaddrinfo: " << gai_strerror(retval) << endl;
		throw runtime_error(s.str());
	}
	if (::connect(this->fd, res->ai_addr, res->ai_addrlen) == -1){
		stringstream s;
		s << "connect: " << strerror(errno) << endl;
		freeaddrinfo(res);
		throw runtime_error(s.str());
	}
	
	freeaddrinfo(res);
}

void mysocket::sendall(string message){
	const char* buf = message.c_str();
	int len = strlen(buf);
	int total = 0;
	int bytesleft = len;
	int n;
	
	while(total < len){
		n = send(this->fd, buf+total, bytesleft, 0);
		if (n == -1) { break; }
		total += n;
		bytesleft -= n;
	}
	
	if (n == -1){
		stringstream s;
		s << "send: " << strerror(errno) << endl;
		throw runtime_error(s.str());
	}
}

string mysocket::recvall(){
	const size_t chunk_size = 4096;
	char buf[chunk_size];
	stringstream result;
	int len;
	
	while ((len = recv(this->fd, buf, chunk_size - 1, 0)) != 0){
		if (len == -1) { break; }
		buf[len] = '\0';
		result << string(buf);
	}
	
	if (len == -1){
		stringstream s;
		s << "recv: " << strerror(errno) << endl;
		throw runtime_error(s.str());
	}
	
	return result.str();
}
