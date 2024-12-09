CC = g++
CFLAGS = -Wall -pthread -std=c++17

all: fileserver fileclient

fileserver: fileserver.o
	$(CC) $(CFLAGS) -o fileserver fileserver.o -lstdc++fs

fileserver.o: fileserver.cpp
	$(CC) $(CFLAGS) -c fileserver.cpp

fileclient: main.o parse.o socket.o
	$(CC) $(CFLAGS) main.o parse.o socket.o -lstdc++fs -o fileclient

main.o: main.cpp
	$(CC) $(CFLAGS) -c main.cpp

parse.o: parse.h parse.cpp
	$(CC) $(CFLAGS) -c parse.cpp

socket.o: socket.cpp socket.h
	$(CC) $(CFLAGS) -c socket.cpp

clean:
	rm -f *.o fileserver fileclient
