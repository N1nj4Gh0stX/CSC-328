CC = g++
CFLAGS = -Wall -pthread -std=c++17

all: fileserver fileclient

fileserver: fileserver.o
	$(CC) $(CFLAGS) -o fileserver fileserver.o -lstdc++fs

fileserver.o: fileserver.cpp
	$(CC) $(CFLAGS) -c fileserver.cpp

fileclient: fileclient.o
	$(CC) $(CFLAGS) -o fileclient fileclient.o

fileclient.o: fileclient.cpp
	$(CC) $(CFLAGS) -c fileclient.cpp

clean:
	rm -f *.o fileserver fileclient


