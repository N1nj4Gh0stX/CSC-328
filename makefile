# Author: Liz Delarosa
# Editor: Arek Gebka
# Filename: Makefile
# Purpose: Automates the compilation of the fileserver and fileclient programs.
#          Ensures proper compilation order, handles dependencies, and provides
#          a clean target for removing generated files.

# Compiler to use for building the programs
CC = g++

# Compiler flags:
#   -Wall: Enable all warnings
#   -pthread: Enable multithreading support
#   -std=c++17: Use the C++17 standard
CFLAGS = -Wall -pthread -std=c++17

# Default target: Builds all executables
all: fileserver fileclient

# Target: fileserver
# Purpose: Compiles and links the fileserver executable
fileserver: fileserver.o socket.o
	$(CC) $(CFLAGS) -o fileserver fileserver.o socket.o -lstdc++fs

# Target: fileserver.o
# Purpose: Compiles the fileserver.cpp source file into an object file
fileserver.o: fileserver.cpp
	$(CC) $(CFLAGS) -c fileserver.cpp

# Target: socket.o
# Purpose: Compiles the socket.cpp source file into an object file
socket.o: socket.cpp socket.h
	$(CC) $(CFLAGS) -c socket.cpp

# Target: fileclient
# Purpose: Compiles and links the fileclient executable
fileclient: fileclient.o clientparse.o socket.o
	$(CC) $(CFLAGS) fileclient.o clientparse.o socket.o -lstdc++fs -o fileclient

# Target: fileclient.o
# Purpose: Compiles the fileclient.cpp source file into an object file
fileclient.o: fileclient.cpp
	$(CC) $(CFLAGS) -c fileclient.cpp

# Target: clientparse.o
# Purpose: Compiles the clientparse.cpp source file into an object file
clientparse.o: clientparse.h clientparse.cpp
	$(CC) $(CFLAGS) -c clientparse.cpp

# Target: clean
# Purpose: Removes all generated files to clean the project directory
clean:
	rm -f *.o fileserver fileclient
