/*************************************************************/
/* author: []                                                */
/* filename: server.cpp                                       */
/* purpose: this source file implements a simple file server */
/*          that allows clients to interact with the server  */
/*          through various commands, such as changing        */
/*          directories, listing files, and transferring files*/
/*          via a custom protocol. The server listens for     */
/*          incoming client connections and handles them in   */
/*          separate processes using `fork`. It supports     */
/*          commands like `get` (to download a file), `put`   */
/*          (to upload a file), `cd`, `pwd`, `ls`, `mkdir`,   */
/*          and `exit`. The server also handles errors and   */
/*          ensures that only files within the specified     */
/*          directory can be accessed.                       */
/*************************************************************/

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <csignal>
#include <unistd.h>
#include <set>
#include <cstring>
#include "socket.h"

using namespace std;
namespace fs = std::filesystem;

string base_directory;
bool shutdown_flag = false;

/*************************************************************/
/* function: sendallFile                                     */
/* purpose: sends a file to the client. It reads the file    */
/*          in chunks and sends the data to the client over  */
/*          the socket connection. The file is checked to    */
/*          ensure it is within the base directory before    */
/*          being sent. The transfer ends with an "EOF"      */
/*          message.                                         */
/* parameters:                                               */
/*    - client: a mysock object representing the client.     */
/*    - file_path: the path of the file to send.             */
/*************************************************************/
void sendallFile(mysock &client, const string &file_path) {
    static set<string> sent_files; // Track sent files for debugging
    cout << "Invoking sendallFile for: " << file_path << endl;

    fs::path absolute_path = fs::absolute(file_path);
    if (absolute_path.string().find(base_directory) != 0) {
        client.clientsend("Error: Access denied.");
        return;
    }

    ifstream infile(file_path, ios::binary);
    if (!infile) {
        client.clientsend("Error: File not found.");
        return;
    }

    char buffer[1024];
    while (infile.read(buffer, sizeof(buffer))) {
        client.clientsend(string(buffer, infile.gcount()));
    }
    if (infile.gcount() > 0) {
        client.clientsend(string(buffer, infile.gcount()));
    }
    infile.close();
    client.clientsend("EOF");
    cout << "File sent: " << file_path << endl;
}

/*************************************************************/
/* function: recvFile                                         */
/* purpose: receives a file from the client and saves it     */
/*          to the specified path on the server. It reads    */
/*          the incoming data in chunks and writes it to a   */
/*          file. The transfer ends when an "EOF" message is */
/*          received.                                        */
/* parameters:                                               */
/*    - client: a mysock object representing the client.     */
/*    - file_path: the path to save the received file.       */
/*************************************************************/
void recvFile(mysock &client, const string &file_path) {
    ofstream outfile(file_path, ios::binary);
    if (!outfile) {
        client.clientsend("Error: Cannot create file.");
        return;
    }

    char buffer[1024];
    int receivedBytes;
    while ((receivedBytes = client.clientrecv(buffer, sizeof(buffer))) > 0) {
        string data(buffer, receivedBytes);
        if (data == "EOF") break;
        outfile.write(data.c_str(), data.size());
    }

    outfile.close();
    cout << "File received: " << file_path << endl;
}

/*************************************************************/
/* function: handleClient                                     */
/* purpose: processes commands received from a client.       */
/*          Supports the following commands:                  */
/*          - exit: disconnects the client.                   */
/*          - cd: changes the current working directory.      */
/*          - pwd: returns the current working directory.    */
/*          - ls: lists the files in the current directory.  */
/*          - mkdir: creates a new directory.                */
/*          - get: sends a file from the server to the client.*/
/*          - put: receives a file from the client.           */
/* parameters:                                               */
/*    - client: the mysock object representing the client.   */
/*************************************************************/
void handleClient(mysock &client) {
    string current_directory = base_directory;

    try {
        while (true) {
            char buffer[1024];
            int receivedBytes = client.clientrecv(buffer, sizeof(buffer) - 1);
            if (receivedBytes <= 0) {
                cout << "Error or client disconnected." << endl;
                break;
            }
            buffer[receivedBytes] = '\0'; // Null-terminate the string
            string command(buffer);

            cout << "Command received: " << command << endl;

            stringstream ss(command);
            string cmd, arg1, arg2;
            ss >> cmd >> arg1 >> arg2;

            if (cmd == "exit") {
                cout << "Client disconnected." << endl;
                break;
            } else if (cmd == "cd") {
                fs::path target_path = fs::absolute(current_directory + "/" + arg1);
                if (fs::exists(target_path) && fs::is_directory(target_path) &&
                    target_path.string().find(base_directory) == 0) {
                    current_directory = target_path.string();
                    client.clientsend("Directory changed to: " + current_directory);
                } else {
                    client.clientsend("Error: Invalid directory.");
                    continue;
                }
            } else if (cmd == "pwd") {
                client.clientsend(current_directory);
                continue;
            } else if (cmd == "ls") {
                fs::path list_path = arg1.empty() ? current_directory : current_directory + "/" + arg1;
                if (!fs::exists(list_path)) {
                    client.clientsend("Error: Path does not exist.");
                    continue;
                } else if (fs::is_directory(list_path)) {
                    stringstream response;
                    for (const auto &entry : fs::directory_iterator(list_path)) {
                        response << entry.path().filename() << (fs::is_directory(entry) ? "/" : "") << "\n";
                    }
                    client.clientsend(response.str());
                } else {
                    client.clientsend("Error: Specified path is not a directory.");
                    continue;
                }
            } else if (cmd == "mkdir") {
                fs::path dir_path = current_directory + "/" + arg1;
                if (fs::create_directory(dir_path)) {
                    client.clientsend("Directory created.");
                } else {
                    client.clientsend("Error: Directory already exists or cannot be created.");
                }
                continue;
            } else if (cmd == "get") {
                cout << "Processing 'get' command for: " << arg1 << endl; // Debug log
                fs::path target_path = fs::absolute(current_directory + "/" + arg1);

                if (!fs::exists(target_path)) {
                    client.clientsend("Error: File or directory does not exist.");
                    continue;
                }

                if (fs::is_regular_file(target_path)) {
                    sendallFile(client, target_path.string());
                    cout << "File sent: " << target_path.string() << endl; // Log the file transfer
                    client.clientsend("EOF"); // Explicitly signal the end of transfer
                } else {
                    client.clientsend("Error: Specified path is not a file.");
                    continue;
                }
            } else if (cmd == "put") {
                fs::path target_path = fs::absolute(current_directory + "/" + arg1);

                if (fs::exists(target_path)) {
                    client.clientsend("Warning: File already exists. Overwriting.");
                    cout << "Overwriting existing file: " << target_path.string() << endl;
                }

                recvFile(client, target_path.string());
                client.clientsend("File received: " + target_path.string());
                cout << "File uploaded: " << target_path.string() << endl; // Log the upload
            } else {
                client.clientsend("Error: Unknown command.");
                continue;
            }
        }
    } catch (const exception &e) {
        cerr << "Error handling client: " << e.what() << endl;
    }
}

/*************************************************************/
/* function: signalHandler                                    */
/* purpose: handles termination signals (e.g., SIGINT) to     */
/*          gracefully shut down the server.                   */
/* parameters:                                               */
/*    - signal: the signal that was received.                 */
/*************************************************************/
void signalHandler(int signal) {
    if (signal == SIGINT) {
        shutdown_flag = true;
        cout << "\nServer will shut down in 5 seconds." << endl;
        sleep(5);
        exit(0);
    }
}

/*************************************************************/
/* function: main                                             */
/* purpose: initializes the server, binds it to a port,      */
/*          and listens for incoming client connections.      */
/*          It processes incoming connections and handles    */
/*          them in separate processes using fork.           */
/* parameters:                                               */
/*    - argc: the number of command-line arguments.          */
/*    - argv: the command-line arguments.                    */
/* exception: throws runtime errors if arguments are missing*/
/*           or invalid.                                      */
/*************************************************************/
int main(int argc, char **argv) {
    if (argc != 5) {
        cerr << "Usage: " << argv[0] << " -p <port> -d <directory>\n";
        return 1;
    }

    string port, directory;
    for (int i = 1; i < argc; i += 2) {
        string arg = argv[i];
        if (arg == "-p") {
            port = argv[i + 1];
        } else if (arg == "-d") {
            directory = argv[i + 1];
        } else {
            cerr << "Unknown argument: " << arg << "\n";
            return 1;
        }
    }

    if (port.empty() || directory.empty()) {
        cerr << "Both port and directory must be specified.\n";
        return 1;
    }

    base_directory = fs::absolute(directory).string();
    if (!fs::exists(base_directory) || !fs::is_directory(base_directory)) {
        cerr << "Error: Specified directory does not exist or is not a directory.\n";
        return 1;
    }

    signal(SIGINT, signalHandler);

    mysock server;
    server.bind(port);
    server.listen(10);

    cout << "Server listening on port " << port << " and serving directory " << base_directory << endl;

    while (!shutdown_flag) {
        mysock client = server.accept();
        cout << "Client connected." << endl;

        if (fork() == 0) { // Handle each client in a new process
            server.close();
            handleClient(client);
            client.close();
            exit(0);
        } else {
            client.close();
        }
    }

    return 0;
}
