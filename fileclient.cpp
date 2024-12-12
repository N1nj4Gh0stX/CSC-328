/*************************************************************/
/* author: []                                                 */
/* filename: client.cpp                                       */
/* purpose: this source file implements the client-side     */
/*          logic for communicating with the server. The     */
/*          client sends commands to the server to interact  */
/*          with files, change directories, and receive or   */
/*          upload files. It supports commands like `exit`,  */
/*          `cd`, `pwd`, `ls`, `mkdir`, `get`, and `put`.    */
/*************************************************************/

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <filesystem>
#include "socket.h"

using namespace std;
namespace fs = std::filesystem;

string base_directory;

/*************************************************************/
/* function: sendFile                                        */
/* purpose: sends a file to the server. The file is read in  */
/*          chunks and sent over the socket connection.      */
/*          The transfer is concluded with an "EOF" message. */
/* parameters:                                               */
/*    - client: the mysock object representing the client.   */
/*    - file_path: the path of the file to send.             */
/*************************************************************/
void sendFile(mysock &client, const string &file_path) {
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
/* function: recvFile                                        */
/* purpose: receives a file from the server and saves it to  */
/*          the specified path on the client. The transfer   */
/*          ends when an "EOF" message is received.          */
/* parameters:                                               */
/*    - client: the mysock object representing the client.   */
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
/* function: handleCommand                                    */
/* purpose: processes the user input command and sends it to */
/*          the server. It supports various commands like    */
/*          `exit`, `cd`, `pwd`, `ls`, `mkdir`, `get`, and    */
/*          `put`. Commands are read and parsed, then        */
/*          sent to the server for execution.                */
/* parameters:                                               */
/*    - client: the mysock object representing the client.   */
/*************************************************************/
void handleCommand(mysock &client) {
    string command;

    try {
        while (true) {
            cout << "> ";
            getline(cin, command);

            if (command.empty()) continue;

            client.clientsend(command);

            stringstream response;
            client.clientrecv(response);

            string response_str = response.str();
            cout << response_str;

            if (command == "exit") {
                break;
            } else if (command == "get" || command == "put") {
                string filename;
                cout << "Enter the filename: ";
                cin >> filename;
                cin.ignore(); // Ignore the newline left by cin

                if (command == "get") {
                    recvFile(client, filename);
                } else {
                    sendFile(client, filename);
                }
            } else {
                continue;
            }
        }
    } catch (const exception &e) {
        cerr << "Error processing command: " << e.what() << endl;
    }
}

/*************************************************************/
/* function: main                                             */
/* purpose: initializes the client, connects to the server,  */
/*          and processes the user's commands interactively. */
/* parameters:                                               */
/*    - argc: the number of command-line arguments.          */
/*    - argv: the command-line arguments.                    */
/* exception: throws runtime errors if arguments are missing*/
/*           or invalid.                                      */
/*************************************************************/
int main(int argc, char **argv) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <server_ip> <port>\n";
        return 1;
    }

    string server_ip = argv[1];
    string port = argv[2];

    mysock client;
    client.connect(server_ip, port);

    cout << "Connected to server at " << server_ip << ":" << port << endl;

    handleCommand(client);

    return 0;
}
