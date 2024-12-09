#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <csignal>
#include <unistd.h>
#include <cstring>
#include "socket.h"

using namespace std;
namespace fs = std::filesystem;

string base_directory;
bool shutdown_flag = false;

void sendallFile(mysock &client, const string &file_path) {
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

void recvFile(mysock &client, const string &file_path) {
    fs::path absolute_path = fs::absolute(file_path);
    if (absolute_path.string().find(base_directory) != 0) {
        client.clientsend("Error: Access denied.");
        return;
    }

    ofstream outfile(file_path, ios::binary);
    if (!outfile) {
        client.clientsend("Error: Cannot create file.");
        return;
    }

    char buffer[1024];
    int receivedBytes;
    while ((receivedBytes = client.clientrecv(buffer, sizeof(buffer))) > 0) {
        outfile.write(buffer, receivedBytes);
    }
    outfile.close();
    cout << "File received: " << file_path << endl;
}

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
                }
            } else if (cmd == "pwd") {
                client.clientsend(current_directory);
            } else if (cmd == "ls") {
                fs::path list_path = arg1.empty() ? current_directory : current_directory + "/" + arg1;
                if (!fs::exists(list_path)) {
                    client.clientsend("Error: Path does not exist.");
                } else if (fs::is_directory(list_path)) {
                    stringstream response;
                    for (const auto &entry : fs::directory_iterator(list_path)) {
                        response << entry.path().filename() << (fs::is_directory(entry) ? "/" : "") << "\n";
                    }
                    client.clientsend(response.str());
                } else {
                    client.clientsend("Error: Specified path is not a directory.");
                }
            } else if (cmd == "mkdir") {
                fs::path dir_path = current_directory + "/" + arg1;
                if (fs::create_directory(dir_path)) {
                    client.clientsend("Directory created.");
                } else {
                    client.clientsend("Error: Directory already exists or cannot be created.");
                }
            } else if (cmd == "get") {
                fs::path target_path = fs::absolute(current_directory + "/" + arg1);

                if (!fs::exists(target_path)) {
                    client.clientsend("Error: File or directory does not exist.");
                } else if (fs::is_directory(target_path)) {
                    if (arg1 == "-R") {
                        client.clientsend("Recursive get not implemented.");
                    } else {
                        client.clientsend("Error: Cannot fetch directory without -R flag.");
                    }
                } else if (fs::is_regular_file(target_path)) {
                    sendallFile(client, target_path.string());
                } else {
                    client.clientsend("Error: Invalid file type.");
                }
            } else if (cmd == "put") {
                recvFile(client, current_directory + "/" + arg1);
            } else {
                client.clientsend("Error: Unknown command.");
            }
        }
    } catch (const exception &e) {
        cerr << "Error handling client: " << e.what() << endl;
    }
}

void signalHandler(int signal) {
    if (signal == SIGINT) {
        shutdown_flag = true;
        cout << "\nServer will shut down in 5 seconds." << endl;
        sleep(5);
        exit(0);
    }
}

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

