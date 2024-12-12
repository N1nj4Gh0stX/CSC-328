/*************************************************************/
/* author: Liz Delarosa                                     */
/* editor: Arek Gebka                                       */
/* filename: Makefile                                       */
/* purpose: This Makefile automates the compilation of the  */
/*          fileserver and fileclient programs. It ensures  */
/*          the correct compilation order, handles          */
/*          dependencies, and provides a clean target to    */
/*          remove generated files.                         */
/*************************************************************/
CC = g++
CFLAGS = -Wall -pthread -std=c++17

all: fileserver fileclient

fileserver: fileserver.o socket.o
	$(CC) $(CFLAGS) -o fileserver fileserver.o socket.o -lstdc++fs

fileserver.o: fileserver.cpp
	$(CC) $(CFLAGS) -c fileserver.cpp

socket.o: socket.cpp socket.h
	$(CC) $(CFLAGS) -c socket.cpp

fileclient: fileclient.o clientparse.o socket.o
	$(CC) $(CFLAGS) fileclient.o clientparse.o socket.o -lstdc++fs -o fileclient

fileclient.o: fileclient.cpp
	$(CC) $(CFLAGS) -c fileclient.cpp

clientparse.o: clientparse.h clientparse.cpp
	$(CC) $(CFLAGS) -c clientparse.cpp

clean:
	rm -f *.o fileserver fileclient
