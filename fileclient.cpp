/*************************************************************/
/* author: Liz Delarosa                                                 */
/* filename: client.cpp                                       */
/* purpose: this source file implements the client-side     */
/*          logic for communicating with the server. The     */
/*          client sends commands to the server to interact  */
/*          with files, change directories, and receive or   */
/*          upload files. It supports commands like `exit`,  */
/*          `cd`, `pwd`, `ls`, `mkdir`, `get`, and `put`.    */
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
#include <thread>

#include "clientparse.h"
#include "socket.h"

using namespace std;
namespace fs = std::filesystem;

/*************************************************************/
/* function: recvallFile                                     */
/* purpose: Receives a file in chunks from the server over   */
/*          a socket connection and writes it to the local   */
/*          file system. The transfer concludes when an      */
/*          "EOF" message is detected.                       */
/* parameters:                                               */
/*    - s: the mysock object representing the server socket. */
/*    - local_file_path: the path where the file will be     */
/*          stored locally.                                  */
/*************************************************************/
void recvallFile(mysock &s, const string &local_file_path) {
    ofstream outFile(local_file_path, ios::binary);
    if (!outFile) {
        cerr << "Error: Cannot create local file " << local_file_path << endl;
        s.clientsend("Error: Cannot create file.");
        return;
    }

    char buffer[1024];
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
/* function: asyncRecvallFile                                */
/* purpose: Initiates an asynchronous file download by       */
/*          spawning a detached thread to handle the         */
/*          recvallFile operation.                          */
/* parameters:                                               */
/*    - s: the mysock object representing the server socket. */
/*    - local_file_path: the path where the file will be     */
/*          stored locally.                                  */
/*************************************************************/
void asyncRecvallFile(mysock &s, const string &local_file_path) {
    thread t(recvallFile, ref(s), ref(local_file_path));
    t.detach();
}

/*************************************************************/
/* function: getRecursive                                    */
/* purpose: Downloads a remote directory recursively by      */
/*          creating local directories and downloading       */
/*          files sent by the server. The directory          */
/*          structure is preserved locally.                  */
/* parameters:                                               */
/*    - s: the mysock object representing the server socket. */
/*    - remote_path: the remote directory to download.       */
/*    - local_path: the local directory to store files.      */
/*************************************************************/
void getRecursive(mysock &s, const string &remote_path, const string &local_path) {
    s.clientsend("get -R " + remote_path);

    char response_buffer[1024];
    int receivedBytes;

    while ((receivedBytes = s.clientrecv(response_buffer, sizeof(response_buffer))) > 0) {
        string response(response_buffer, receivedBytes);

        if (response == "EOF") {
            cout << "Server signaled end of directory listing." << endl;
            break;
        }

        stringstream ss(response);
        string type, relative_path;
        ss >> type >> relative_path;

        string local_file_path = local_path + "/" + relative_path;

        if (type == "DIR") {
            try {
                fs::create_directories(local_file_path);
                cout << "Directory created: " << local_file_path << endl;
            } catch (const fs::filesystem_error &e) {
                cerr << "Error creating local directory: " << e.what() << endl;
                continue;
            }
        } else if (type == "FILE") {
            cout << "Fetching file: " << relative_path << " -> " << local_file_path << endl;
            s.clientsend("get " + relative_path);
            asyncRecvallFile(s, local_file_path);
        } else {
            cerr << "Unknown type received: " << type << endl;
            continue;
        }
    }

    if (receivedBytes <= 0) {
        cerr << "Error: Connection to the server lost or server closed unexpectedly." << endl;
    }

    cout << "Directory download complete: " << local_path << endl;
}

/*************************************************************/
/* function: asyncPutFile                                    */
/* purpose: Initiates an asynchronous file upload by         */
/*          spawning a detached thread to send the file to   */
/*          the server in chunks, concluding with an "EOF"   */
/*          message.                                         */
/* parameters:                                               */
/*    - s: the mysock object representing the server socket. */
/*    - local_file_path: the local file path to upload.      */
/*    - remote_file_path: the remote path on the server.     */
/*************************************************************/
void asyncPutFile(mysock &s, const string &local_file_path, const string &remote_file_path) {
    thread t([&]() {
        ifstream infile(local_file_path, ios::binary);
        if (!infile.is_open()) {
            cerr << "Error opening local file: " << local_file_path << endl;
            return;
        }

        s.clientsend("put " + remote_file_path);
        char buffer[1024];
        while (infile.read(buffer, sizeof(buffer))) {
            s.clientsend(string(buffer, infile.gcount()));
        }
        infile.close();
        s.clientsend("EOF");
        cout << "File uploaded: " << local_file_path << " -> " << remote_file_path << endl;
    });
    t.detach();
}

/*************************************************************/
/* function: putRecursive                                    */
/* purpose: Uploads a local directory recursively to the     */
/*          server by creating remote directories and        */
/*          uploading files while preserving directory       */
/*          structure.                                       */
/* parameters:                                               */
/*    - s: the mysock object representing the server socket. */
/*    - local_path: the local directory to upload.           */
/*    - remote_path: the remote directory path on the server.*/
/*************************************************************/
void putRecursive(mysock &s, const string &local_path, const string &remote_path) {
    s.clientsend("mkdir " + remote_path);
    char buffer[1024];
    int receivedBytes = s.clientrecv(buffer, sizeof(buffer));

    string response(buffer, receivedBytes);
    if (response == "Error") {
        cerr << "Error creating remote directory: " << remote_path << endl;
        return;
    }

    for (const auto &entry : fs::recursive_directory_iterator(local_path)) {
        string local_file_path = entry.path().string();
        string relative_path = fs::relative(entry.path(), local_path).string();
        string remote_file_path = remote_path + "/" + relative_path;

        if (entry.is_directory()) {
            s.clientsend("mkdir " + remote_file_path);
            s.clientrecv(buffer, sizeof(buffer));
            cout << "Remote directory created: " << remote_file_path << endl;
        } else if (entry.is_regular_file()) {
            asyncPutFile(s, local_file_path, remote_file_path);
        } else {
            cout << "Skipping unsupported file type: " << local_file_path << endl;
        }
    }

    cout << "Directory upload complete: " << local_path << endl;
}

/*************************************************************/
/* function: printLocalWorkingDirectory                      */
/* purpose: Prints the current working directory of the      */
/*          client to the console.                           */
/* parameters: None                                          */
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
/* function: displayHelp                                     */
/* purpose: Displays a list of available commands and their  */
/*          descriptions to the console.                     */
/* parameters: None                                          */
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
/* function: handleServerMessage                             */
/* purpose: Processes messages received from the server,     */
/*          handling special cases like server shutdown      */
/*          notifications, and prints the messages.          */
/* parameters:                                               */
/*    - response: the message received from the server.      */
/*    - receivedBytes: the size of the received message.     */
/*************************************************************/
void handleServerMessage(const char *response, int receivedBytes) {
    string server_message(response, receivedBytes);

    if (server_message.find("Server will shut down in 5 seconds.") != string::npos) {
        cout << server_message << endl;
        cout << "Client will shut down in 5 seconds..." << endl;
        sleep(5);
        exit(0); // Graceful exit
    }

    // Otherwise, just print the server's message
    cout << server_message << endl;
}

/*************************************************************/
/* function: main                                            */
/* purpose: Entry point for the client program. Initializes  */
/*          the client socket connection and handles the     */
/*          command-line interface for interacting with the  */
/*          server, processing commands like `exit`, `cd`,   */
/*          `get`, `put`, and others.                        */
/* parameters:                                               */
/*    - argc: the number of command-line arguments.          */
/*    - argv: the array of command-line arguments.           */
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
        }

        try {
            if (command == "lcd") {
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
            } else if (command == "lls") {
                if (argument.empty()) {
                    argument = "."; // Default to current directory
                }

                DIR *dir = opendir(argument.c_str());
                if (dir) {
                    struct dirent *entry;
                    while ((entry = readdir(dir)) != nullptr) {
                        cout << entry->d_name << endl;
                    }
                    closedir(dir);
                } else {
                    perror("Error listing local directory");
                }
            } else if (command == "lmkdir") {
                if (argument.empty()) {
                    cout << "Error: Path not specified." << endl;
                } else {
                    if (mkdir(argument.c_str(), 0755) == 0) {
                        cout << "Local directory created: " << argument << endl;
                    } else {
                        perror("Error creating local directory");
                    }
                }
            } else if (command == "help") {
                displayHelp();
            } else if (command == "cd") {
                s.clientsend("cd " + argument);
                char response[1024];
                int receivedBytes = s.clientrecv(response, sizeof(response));
                if (receivedBytes <= 0) {
                    cerr << "Error: Connection to the server lost during 'cd' command." << endl;
                    break;
                }
                handleServerMessage(response, receivedBytes);
            } else if (command == "pwd") {
                s.clientsend("pwd");
                char response[1024];
                int receivedBytes = s.clientrecv(response, sizeof(response));
                if (receivedBytes <= 0) {
                    cerr << "Error: Connection to the server lost during 'pwd' command." << endl;
                    break;
                }
                handleServerMessage(response, receivedBytes);
            } else if (command == "ls") {
                s.clientsend("ls " + argument);
                char response[1024];
                int receivedBytes = s.clientrecv(response, sizeof(response));
                if (receivedBytes <= 0) {
                    cerr << "Error: Connection lost to the server during 'ls' command." << endl;
                    break;
                }
                handleServerMessage(response, receivedBytes);
            } else if (command == "mkdir") {
                s.clientsend("mkdir " + argument);
                char response[1024];
                int receivedBytes = s.clientrecv(response, sizeof(response));
                if (receivedBytes <= 0) {
                    cerr << "Error: Connection lost to the server during 'mkdir' command." << endl;
                    break;
                }
                handleServerMessage(response, receivedBytes);
            } else if (command == "get") {
                size_t flag_pos = argument.find("-R");
                bool recursive = (flag_pos != string::npos);

                if (recursive) {
                    remote_path = argument.substr(flag_pos + 3); // Skip the `-R` flag
                    local_path = remote_path;
                } else {
                    remote_path = argument;
                    local_path = argument;
                }

                size_t space_pos = remote_path.find_last_of(' ');
                if (space_pos != string::npos) {
                    local_path = remote_path.substr(space_pos + 1);
                    remote_path = remote_path.substr(0, space_pos);
                }

                if (remote_path.empty()) {
                    cout << "Error: Remote path not specified.\n";
                    continue;
                }

                if (recursive) {
                    getRecursive(s, remote_path, local_path);
                } else {
                    s.clientsend("get " + remote_path);
                    recvallFile(s, local_path);
                }
            } else if (command == "put") {
                size_t flag_pos = argument.find("-R");
                bool recursive = (flag_pos != string::npos);

                if (recursive) {
                    local_path = argument.substr(flag_pos + 3); // Skip `-R`
                    remote_path = local_path;
                } else {
                    local_path = argument;
                    remote_path = argument;
                }

                size_t space_pos = local_path.find_last_of(' ');
                if (space_pos != string::npos) {
                    remote_path = local_path.substr(space_pos + 1);
                    local_path = local_path.substr(0, space_pos);
                }

                if (local_path.empty()) {
                    cout << "Error: Local path not specified.\n";
                    continue;
                }

                if (fs::is_directory(local_path)) {
                    if (recursive) {
                        putRecursive(s, local_path, remote_path);
                    } else {
                        cout << "Error: Local path is a directory. Use -R for recursive upload.\n";
                    }
                } else if (fs::is_regular_file(local_path)) {
                    s.clientsend("put " + remote_path);
                    ifstream infile(local_path, ios::binary);
                    char buffer[1024];
                    while (infile.read(buffer, sizeof(buffer))) {
                        s.clientsend(string(buffer, infile.gcount()));
                    }
                    infile.close();
                    s.clientsend("EOF");
                    cout << "File uploaded: " << local_path << " -> " << remote_path << endl;
                } else {
                    cout << "Error: Invalid local path.\n";
                }
            } else {
                cout << "Error: Unknown command." << endl;
            }
        } catch (const exception &e) {
            cerr << "Error: " << e.what() << endl;
            break;
        }
    }

    s.close();
    cout << "Client will be terminated in 5 seconds." << endl;
    sleep(5);
    return 0;
}

