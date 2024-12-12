/*************************************************************/
/* Author: Lizmary Delarosa                                  */
/* Editor: Arek Gebka                                 	     */
/* Major: Computer Science                                  */
/* Creation Date: November 11, 2024                         */
/* Due Date: November 15, 2024                              */
/* Course: CPSC 328                                         */
/* Professor Name: Dr. Dylan Schwesinger                    */
/* Assignment: Network Program Implementation               */
/* filename: fileserver.cpp                                    */
/* Purpose: This file implements the client-side of the file */
/*          transfer program using sockets. It includes       */
/*          functions for sending and receiving files,       */
/*          handling directories, and executing commands.    */
/*************************************************************/
/*************************************************************/
/* Citations:                                                */
/* [1] Linux Man Pages, "socket(2) - Linux manual page,"     */
/*     https://man7.org/linux/man-pages/man2/socket.2.html   */
/* [2] Brian "Beej" Hall, "Beej's Guide to Network           */
/*     Programming," https://beej.us/guide/bgnet/            */
/* [3] cppreference.com, "C++ Standard Library reference,"   */
/*     https://en.cppreference.com/w/                        */
/* [4] cplusplus.com, "std::filesystem - C++ Reference,"     */
/*     https://cplusplus.com/reference/filesystem/           */
/* [6] W. Richard Stevens, "UNIX Network Programming,"       */
/*     Prentice Hall, 2003.                                  */
/* [7] Dr. Dylan Schwesinger, Server Project Solutions       */
/*     and examples in CPSC 328 course materials.            */
/* [8] Arek Gebka, "webclient.c" and other files for         */
/*     solutions to Projects 3 and 5 of CPSC 328 course."    */
/* [9] Lizmary Delarosa, "webclient.c" and other files for   */
/*     solutions to Projects 3 and 5 of CPSC 328 course."    */
/*************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <set>
#include <dirent.h>
#include <sys/stat.h>
#include <stack>
#include <filesystem>

#include "clientparse.h"
#include "socket.h"

// Define default buffer size for file transfer
constexpr size_t DEFAULT_BUFFER_SIZE = 4096;

using namespace std;
namespace fs = std::filesystem;

/*************************************************************/
/* Function: recvallFile                                      */
/* Purpose: Receives the entire contents of a file from the  */
/*          server and writes it to a local file.            */
/* Input: s - The socket object used for communication.      */
/*        local_file_path - The path to the local file where*/
/*        the received file will be saved.                   */
/*************************************************************/
void recvallFile(mysock &s, const string &local_file_path) {
    ofstream outFile(local_file_path, ios::binary);
    if (!outFile) {
        cerr << "Error: Cannot create local file " << local_file_path << endl;
        s.clientsend("Error: Cannot create file.");
        return;
    }

    char buffer[DEFAULT_BUFFER_SIZE];
    while (true) {
        int receivedBytes = s.clientrecv(buffer, sizeof(buffer));
        if (receivedBytes <= 0) {
            cerr << "Error: Connection lost or server error during file transfer." << endl;
            break;
        }

        string data(buffer, receivedBytes);

        // Detect EOF explicitly
        if (data == "EOF") {
            cout << "File transfer completed: " << local_file_path << endl;
            break;
        }

        // Write received data to file
        outFile.write(data.c_str(), data.size());
    }

    outFile.close();
    if (!outFile) {
        cerr << "Error: Writing to file failed." << endl;
    }
}

/*************************************************************/
/* Function: getRecursive                                     */
/* Purpose: Recursively retrieves a directory and its files */
/*          from the server and saves them locally.          */
/* Input: s - The socket object used for communication.      */
/*        remote_path - The path to the remote directory to  */
/*        be retrieved.                                      */
/*        local_path - The local path where the directory    */
/*        and files will be saved.                           */
/*************************************************************/
void getRecursive(mysock &s, const string &remote_path, const string &local_path) {
    // Send the recursive get request to the server
    s.clientsend("get -R " + remote_path);

    char response_buffer[DEFAULT_BUFFER_SIZE];
    int receivedBytes;

    // Process server responses
    while ((receivedBytes = s.clientrecv(response_buffer, sizeof(response_buffer))) > 0) {
        string response(response_buffer, receivedBytes);

        // End of directory listing
        if (response == "EOF") {
            cout << "Server signaled end of directory listing." << endl;
            break;
        }

        stringstream ss(response);
        string type, relative_path;
        ss >> type >> relative_path;

        string local_file_path = local_path + "/" + relative_path;

        if (type == "DIR") {
            // Create the corresponding local directory
            try {
                fs::create_directories(local_file_path);
                cout << "Directory created: " << local_file_path << endl;
            } catch (const fs::filesystem_error &e) {
                cerr << "Error creating local directory: " << e.what() << endl;
                continue; // Skip to the next item
            }
        } else if (type == "FILE") {
            // Fetch the file from the server
            cout << "Fetching file: " << relative_path << " -> " << local_file_path << endl;
            s.clientsend("get " + relative_path);
            recvallFile(s, local_file_path); // Corrected call
        } else {
            cerr << "Unknown type received: " << type << endl;
            continue; // Skip invalid responses
        }
    }

    // Detect socket closure or end of response
    if (receivedBytes <= 0) {
        cerr << "Error: Connection lost or server closed unexpectedly." << endl;
    }

    cout << "Directory download complete: " << local_path << endl;
}

/*************************************************************/
/* Function: putRecursive                                    */
/* Purpose: Recursively uploads a directory and its files   */
/*          to the server.                                   */
/* Input: s - The socket object used for communication.      */
/*        local_path - The path to the local directory to    */
/*        be uploaded.                                       */
/*        remote_path - The path to the remote directory.    */
/*************************************************************/
void putRecursive(mysock &s, const string &local_path, const string &remote_path) {
    // Ensure the remote directory exists
    s.clientsend("mkdir " + remote_path);
    char buffer[DEFAULT_BUFFER_SIZE];
    int receivedBytes = s.clientrecv(buffer, sizeof(buffer));

    string response(buffer, receivedBytes);
    if (response == "Error") {
        cout << "Error creating remote directory: " << remote_path << endl;
        return;
    }

    // Iterate through the local directory structure
    for (const auto &entry : fs::recursive_directory_iterator(local_path)) {
        string local_file_path = entry.path().string();
        string relative_path = fs::relative(entry.path(), local_path).string();
        string remote_file_path = remote_path + "/" + relative_path;

        if (entry.is_directory()) {
            // Create the corresponding remote directory
            s.clientsend("mkdir " + remote_file_path);
            s.clientrecv(buffer, sizeof(buffer)); // Ignore mkdir response
            cout << "Remote directory created: " << remote_file_path << endl;
        } else if (entry.is_regular_file()) {
            // Upload the file
            ifstream infile(local_file_path, ios::binary);
            if (!infile.is_open()) {
                cout << "Error opening local file: " << local_file_path << endl;
                continue;
            }

            s.clientsend("put " + remote_file_path);
            while (infile.read(buffer, sizeof(buffer))) {
                s.clientsend(string(buffer, infile.gcount()));
            }
            infile.close();
            s.clientsend("EOF");
            cout << "File uploaded: " << local_file_path << " -> " << remote_file_path << endl;
        } else {
            cout << "Skipping unsupported file type: " << local_file_path << endl;
        }
    }

    cout << "Directory upload complete: " << local_path << endl;
}

/*************************************************************/
/* Function: printLocalWorkingDirectory                      */
/* Purpose: Prints the current local working directory.      */
/*************************************************************/
void printLocalWorkingDirectory() {
	char cwd[PATH_MAX]; // PATH_MAX is the maximum length for paths
	if (getcwd(cwd, sizeof(cwd)) != nullptr) {  // get the current working directory
		cout << "Local working directory: " << cwd << endl;
	} 
	else {
		perror("Error getting local working directory");
	}
}

/*************************************************************/
/* Function: displayHelp                                      */
/* Purpose: Displays a list of available commands and their   */
/*          descriptions to help the user.                    */
/*************************************************************/
void displayHelp() {
	cout << "Available commands:\n"
	 << "exit - Quit the application.\n"
	 << "cd [path] - Change remote directory.\n"
	 << "get [-R] remote-path [local-path] - Retrieve remote file/directory.\n"
	 << "help - Display this help text.\n"
	 << "lcd [path] - Change local directory.\n"
	 << "lls [path] - List local directory contents.\n"
	 << "lmkdir path - Create local directory.\n"
	 << "lpwd - Display local working directory.\n"
	 << "ls [path] - List remote directory contents.\n"
	 << "mkdir path - Create remote directory.\n"
	 << "put [-R] local-path [remote-path] - Upload file/directory.\n"
	 << "pwd - Display remote working directory.\n";
}

/*************************************************************/
/* Main Function:                                              */
/* Purpose: Main entry point of the client program. This      */
/*          function handles user commands, interacts with    */
/*          the server, and manages file and directory        */
/*          transfers.                                        */
/*************************************************************/
int main(int argc, char **argv) {
    struct options o = parsemenu(argc, argv);

    mysock s;
    s.connect(o.hostname, o.port);
    cout << "Connected to server." << endl;

    string command, argument, remote_path, local_path;

    // REPL loop
    while (true) {
        cout << "client> ";
        string input;
        getline(cin, input);

        if (input.empty()) {
            continue;
        }

        size_t len = input.find(' ');
        command = input.substr(0, len); // Extract the command
        argument = (len != string::npos) ? input.substr(len + 1) : "";

        if (command == "exit") {
            s.clientsend("exit");
            cout << "Exiting...\n";
            break;
        } else if (command == "lcd") {
            if (argument.empty()) {
                argument = getenv("HOME"); // Default to home directory
            }
            if (chdir(argument.c_str()) == 0) {
                cout << "Local directory changed to: " << argument << endl;
            } else {
                perror("Error changing local directory");
            }
        } else if (command == "lpwd") {
            printLocalWorkingDirectory();
        } else if (command == "help") {
            displayHelp();
        } else if (command == "cd") {
            s.clientsend("cd " + argument);
            char response[DEFAULT_BUFFER_SIZE];
            int receivedBytes = s.clientrecv(response, sizeof(response));
            cout << string(response, receivedBytes) << endl;
        } else if (command == "pwd") {
            s.clientsend("pwd");
            char response[DEFAULT_BUFFER_SIZE];
            int receivedBytes = s.clientrecv(response, sizeof(response));
            cout << "Remote directory: " << string(response, receivedBytes) << endl;
        } else if (command == "lls") {
            // Handle local ls
            DIR *dir = opendir(argument.empty() ? "." : argument.c_str());
            if (dir) {
                struct dirent *entry;
                while ((entry = readdir(dir)) != nullptr) {
                    cout << entry->d_name << (entry->d_type == DT_DIR ? "/" : "") << "\n";
                }
                closedir(dir);
            } else {
                perror("Error listing local directory");
            }
        } else if (command == "ls") {
            s.clientsend("ls " + argument);
            char response[DEFAULT_BUFFER_SIZE];
            int receivedBytes = s.clientrecv(response, sizeof(response));
            cout << string(response, receivedBytes) << endl;
        } else if (command == "mkdir") {
            s.clientsend("mkdir " + argument);
            char response[DEFAULT_BUFFER_SIZE];
            int receivedBytes = s.clientrecv(response, sizeof(response));
            cout << string(response, receivedBytes) << endl;
        } else if (command == "lmkdir") {
            if (argument.empty()) {
                cout << "Error: Directory name not specified.\n";
                continue;
            }
            // Attempt to create the directory
            if (mkdir(argument.c_str(), 0755) == 0) {
                cout << "Directory created: " << argument << endl;
            } else {
                perror("Error creating directory");
            }
        } else if (command == "put") {
            size_t position = argument.find(" -R");
            if (position != string::npos) {
                local_path = argument.substr(0, position);
                remote_path = argument.substr(position + 4);
                putRecursive(s, local_path, remote_path);
            } else {
                s.clientsend("put " + argument);
                // Implement the file sending functionality here
            }
        } else if (command == "get") {
            size_t position = argument.find(" -R");
            if (position != string::npos) {
                local_path = argument.substr(position + 4);
                remote_path = argument.substr(0, position);
                getRecursive(s, remote_path, local_path);
            } else {
                recvallFile(s, argument); // Fetch single file
            }
        } else {
            cout << "Unknown command: " << command << endl;
        }
    }

    s.close();
    return 0;
}
