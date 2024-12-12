/*************************************************************/
/* author: Arek Gebka                                       */
/* filename: fileserver.cpp                                 */
/* purpose: Implements a secure file server that handles    */
/*          file and directory commands from a client.      */
/*          The server supports commands such as file       */
/*          uploads/downloads, directory navigation, and    */
/*          listing directory contents, while ensuring all  */
/*          actions are restricted to a base directory.     */
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
#include <dirent.h>
#include <sys/stat.h>
#include <vector>
#include <algorithm>

using namespace std;
namespace fs = std::filesystem;

constexpr size_t DEFAULT_BUFFER_SIZE = 4096;
constexpr int SUCCESS_CODE = 0;
const vector<string> ALLOWED_EXTENSIONS = {".txt", ".csv", ".log"};

string base_directory;
bool shutdown_flag = false;

/*************************************************************/
/* function: isWithinBaseDirectory                          */
/* purpose: Verifies whether a given file path is within    */
/*          the allowed base directory to ensure security.  */
/* parameters:                                              */
/*    - path: the file or directory path to check.          */
/*************************************************************/
bool isWithinBaseDirectory(const fs::path &path) {
    try {
        fs::path canonical_base = fs::canonical(base_directory);
        fs::path canonical_path = fs::weakly_canonical(path);
        return canonical_path.string().find(canonical_base.string()) == 0;
    } catch (const fs::filesystem_error &e) {
        return false;
    }
}

/*************************************************************/
/* function: hasAllowedExtension                            */
/* purpose: Checks if a file path has an allowed extension  */
/* parameters:                                              */
/*    - file_path: the file path to check.                  */
/*************************************************************/
bool hasAllowedExtension(const fs::path &file_path) {
    string ext = file_path.extension().string();
    return find(ALLOWED_EXTENSIONS.begin(), ALLOWED_EXTENSIONS.end(), ext) != ALLOWED_EXTENSIONS.end();
}

/*************************************************************/
/* function: sendallFile                                    */
/* purpose: Sends a file to the client in chunks over a     */
/*          socket connection. Appends an "EOF" marker to   */
/*          signal the end of the transfer.                 */
/* parameters:                                              */
/*    - client: the mysock object representing the client.  */
/*    - file_path: the path of the file to be sent.         */
/*************************************************************/
void sendallFile(mysock &client, const string &file_path) {
    if (!hasAllowedExtension(file_path)) {
        client.clientsend("Error: Unsupported file type.");
        return;
    }

    ifstream infile(file_path, ios::binary);
    if (!infile) {
        client.clientsend("Error: File not found.");
        return;
    }

    vector<char> buffer(DEFAULT_BUFFER_SIZE);
    while (infile.read(buffer.data(), buffer.size()) || infile.gcount() > 0) {
        client.clientsend(string(buffer.data(), infile.gcount()));
    }
    infile.close();
    client.clientsend("EOF");
}

/*************************************************************/
/* function: recvFile                                       */
/* purpose: Receives a file from the client and writes it   */
/*          to the server's file system. The operation ends */
/*          when an "EOF" marker is received. Ensures all   */
/*          bytes are written to the file.                  */
/* parameters:                                              */
/*    - client: the mysock object representing the client.  */
/*    - file_path: the destination path for the file.       */
/*************************************************************/
void recvFile(mysock &client, const string &file_path) {
    if (!hasAllowedExtension(file_path)) {
        client.clientsend("Error: Unsupported file type.");
        return;
    }

    ofstream outfile(file_path, ios::binary);
    if (!outfile) {
        client.clientsend("Error: Cannot create file.");
        return;
    }

    vector<char> buffer(DEFAULT_BUFFER_SIZE);
    while (true) {
        int receivedBytes = client.clientrecv(buffer.data(), buffer.size());
        if (receivedBytes <= 0) {
            break;
        }
        string data(buffer.data(), receivedBytes);
        if (data == "EOF") {
            break;
        }
        outfile.write(data.c_str(), receivedBytes);
    }

    outfile.close();
    client.clientsend("File received successfully.");
}

/*************************************************************/
/* function: parseCommands                                  */
/* purpose: Parses and executes multiple commands received  */
/*          in a single buffer.                             */
/* parameters:                                              */
/*    - commands: the raw commands string.                  */
/*************************************************************/
vector<string> parseCommands(const string &commands) {
    vector<string> command_list;
    stringstream ss(commands);
    string command;
    while (getline(ss, command, '\n')) {
        command_list.push_back(command);
    }
    return command_list;
}

/*************************************************************/
/* function: handleClient                                   */
/* purpose: Processes commands from the client, such as     */
/*          file uploads/downloads, directory navigation,   */
/*          and listing contents. Each command is executed  */
/*          securely within the base directory.             */
/* parameters:                                              */
/*    - client: the mysock object representing the client.  */
/*************************************************************/
void handleClient(mysock &client) {
    string current_directory = base_directory;

    try {
        while (true) {
            char buffer[DEFAULT_BUFFER_SIZE];
            int receivedBytes = client.clientrecv(buffer, sizeof(buffer) - 1);
            if (receivedBytes <= 0) {
                cout << "Error or client disconnected." << endl;
                break;
            }
            buffer[receivedBytes] = '\0';
            vector<string> commands = parseCommands(buffer);

            for (const auto &command : commands) {
                cout << "Command received: " << command << endl;

                stringstream ss(command);
                string cmd, arg1, arg2;
                ss >> cmd >> arg1 >> arg2;
                
                //comand handling logic
                if (cmd == "exit") {
                    cout << "Client disconnected." << endl;
                    break;
                } else if (cmd == "cd") {
                    fs::path target_path = fs::absolute(current_directory + "/" + arg1);
                    if (!fs::exists(target_path)) {
                        client.clientsend("Error: Directory does not exist.");
                    } else if (!isWithinBaseDirectory(target_path)) {
                        client.clientsend("Error: Access denied to restricted directory.");
                    } else if (fs::is_directory(target_path)) {
                        current_directory = target_path.string();
                        client.clientsend("Directory changed to: " + current_directory);
                    } else {
                        client.clientsend("Error: Target is not a directory.");
                    }
                    continue;
                } else if (cmd == "pwd") {
                    client.clientsend(current_directory);
                    continue;
                } else if (cmd == "ls") {
                    fs::path list_path = arg1.empty() ? current_directory : current_directory + "/" + arg1;
                    if (!fs::exists(list_path) || !isWithinBaseDirectory(list_path)) {
                        client.clientsend("Error: Path does not exist or access denied.");
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
                    fs::path dir_path = fs::absolute(current_directory + "/" + arg1);
                    if (isWithinBaseDirectory(dir_path)) {
                        try {
                            if (fs::create_directory(dir_path)) {
                                client.clientsend("Directory created.");
                            } else {
                                client.clientsend("Error: Directory already exists or cannot be created.");
                            }
                        } catch (const fs::filesystem_error &e) {
                            client.clientsend("Error: " + string(e.what()));
                        }
                    } else {
                        client.clientsend("Error: Access denied.");
                    }
                    continue;
                } else if (cmd == "lmkdir") {
                    if (arg1.empty()) {
                        client.clientsend("Error: Directory name not specified.");
                        continue;
                    }
                    if (mkdir(arg1.c_str(), 0755) == 0) {
                        client.clientsend("Local directory created: " + arg1);
                    } else {
                        client.clientsend("Error: Unable to create local directory.");
                    }
                    continue;
                } else if (cmd == "lls") {
                    DIR *dir = opendir(arg1.empty() ? "." : arg1.c_str());
                    if (dir) {
                        stringstream response;
                        struct dirent *entry;
                        while ((entry = readdir(dir)) != nullptr) {
                            response << entry->d_name << (entry->d_type == DT_DIR ? "/" : "") << "\n";
                        }
                        closedir(dir);
                        client.clientsend(response.str());
                    } else {
                        client.clientsend("Error: Unable to list local directory.");
                    }
                    continue;
                } else if (cmd == "get") {
                    cout << "Processing 'get' command for: " << arg1 << endl;
                    fs::path target_path = fs::absolute(current_directory + "/" + arg1);

                    if (!fs::exists(target_path) || !isWithinBaseDirectory(target_path)) {
                        client.clientsend("Error: File or directory does not exist or access denied.");
                        continue;
                    }

                    if (fs::is_regular_file(target_path)) {
                        sendallFile(client, target_path.string());
                        cout << "File sent: " << target_path.string() << endl;
                        client.clientsend("EOF");
                    } else {
                        client.clientsend("Error: Specified path is not a file.");
                        continue;
                    }
                } else if (cmd == "put") {
                    fs::path target_path = fs::absolute(current_directory + "/" + arg1);

                    if (!isWithinBaseDirectory(target_path)) {
                        client.clientsend("Error: Access denied.");
                        continue;
                    }

                    if (fs::exists(target_path)) {
                        client.clientsend("Warning: File already exists. Overwriting.");
                        cout << "Overwriting existing file: " << target_path.string() << endl;
                    }

                    recvFile(client, target_path.string());
                    client.clientsend("File received: " + target_path.string());
                    cout << "File uploaded: " << target_path.string() << endl;
                } else {
                    client.clientsend("Error: Unknown command.");
                    continue;
                }
            }
        }
    } catch (const exception &e) {
        cerr << "Error handling client: " << e.what() << endl;
    }
}
/*************************************************************/
/* function: signalHandler                                  */
/* purpose: Handles SIGINT signals to gracefully shut down  */
/*          the server after a delay.                       */
/* parameters:                                              */
/*    - signal: the signal number received.                 */
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
/* function: main                                           */
/* purpose: The entry point for the server application.     */
/*          Initializes the server, binds to a specified    */
/*          port, and sets the base directory for file      */
/*          operations. Handles incoming client connections */
/*          and spawns child processes for each client.     */
/* parameters:                                              */
/*    - argc: the number of command-line arguments.         */
/*    - argv: the array of command-line arguments.          */
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

        if (fork() == 0) {
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
