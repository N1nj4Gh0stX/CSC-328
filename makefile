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

/*************************************************************/
/* Variable: CC                                              */
/* purpose: Specifies the compiler to use for C++ files.     */
/* default: g++                                              */
/*************************************************************/

/*************************************************************/
/* Variable: CFLAGS                                          */
/* purpose: Defines the compiler flags for all targets.      */
/* options:                                                 */
/*    -Wall: Enables all warnings.                          */
/*    -pthread: Enables multithreading support.             */
/*    -std=c++17: Specifies the C++17 standard.             */
/*************************************************************/

/*************************************************************/
/* Target: all                                               */
/* purpose: Builds all executable targets in the project.    */
/* dependencies: fileserver, fileclient                     */
/*************************************************************/

/*************************************************************/
/* Target: fileserver                                        */
/* purpose: Compiles and links the fileserver executable.    */
/* dependencies: fileserver.o, socket.o                     */
/*************************************************************/

/*************************************************************/
/* Target: fileserver.o                                      */
/* purpose: Compiles the fileserver.cpp source file into     */
/*          an object file.                                  */
/* dependencies: fileserver.cpp                             */
/*************************************************************/

/*************************************************************/
/* Target: socket.o                                          */
/* purpose: Compiles the socket.cpp source file into an      */
/*          object file.                                     */
/* dependencies: socket.cpp, socket.h                       */
/*************************************************************/

/*************************************************************/
/* Target: fileclient                                        */
/* purpose: Compiles and links the fileclient executable.    */
/* dependencies: fileclient.o, clientparse.o, socket.o       */
/*************************************************************/

/*************************************************************/
/* Target: fileclient.o                                      */
/* purpose: Compiles the fileclient.cpp source file into an  */
/*          object file.                                     */
/* dependencies: fileclient.cpp                             */
/*************************************************************/

/*************************************************************/
/* Target: clientparse.o                                     */
/* purpose: Compiles the clientparse.cpp source file into an */
/*          object file.                                     */
/* dependencies: clientparse.cpp, clientparse.h             */
/*************************************************************/

/*************************************************************/
/* Target: clean                                             */
/* purpose: Removes all generated files, including object    */
/*          files and executables, to clean the project      */
/*          directory.                                       */
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
