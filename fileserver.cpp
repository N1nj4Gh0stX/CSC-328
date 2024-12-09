#include <iostream>
#include <string>
#include <experimental/filesystem>
#include <thread>
#include <vector>
#include <cstring>
#include <csignal>
#include <arpa/inet.h>
#include <unistd.h>
#include <mutex>
#include <fstream>
#include <algorithm>

using namespace std;
namespace fs = std::experimental::filesystem;

#define BUFFER_SIZE 1024

// Global variables
bool serverRunning = true;
vector<int> clientSockets;
mutex clientMutex;
int serverSocket;

void signalHandler(int signum) {
    cout << "\nServer shutting down in 5 seconds..." << endl;
    sleep(5);

    clientMutex.lock();
    for (int clientSocket : clientSockets) {
        send(clientSocket, "Server is shutting down", strlen("Server is shutting down"), 0);
        close(clientSocket);
    }
    clientSockets.clear();
    clientMutex.unlock();

    close(serverSocket);
    exit(signum);
}

void handleClient(int clientSocket, const fs::path& baseDirectory) {
    char buffer[BUFFER_SIZE];
    while (serverRunning) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0) {
            cout << "Client disconnected." << endl;
            break;
        }

        string command(buffer);
        cout << "Received command: " << command << endl;

        istringstream commandStream(command);
        string action;
        commandStream >> action;

        if (action == "exit") {
            send(clientSocket, "Goodbye", strlen("Goodbye"), 0);
            break;
        } else if (action == "pwd") {
            string currentPath = baseDirectory.string();
            send(clientSocket, currentPath.c_str(), currentPath.length(), 0);
        } else if (action == "read") {
            string filePath;
            commandStream >> filePath;
            fs::path fullPath = baseDirectory / filePath;

            if (fs::exists(fullPath) && fs::is_regular_file(fullPath)) {
                ifstream file(fullPath);
                stringstream fileContent;
                fileContent << file.rdbuf();
                string content = fileContent.str();
                send(clientSocket, content.c_str(), content.length(), 0);
            } else {
                string response = "File not found.";
                send(clientSocket, response.c_str(), response.length(), 0);
            }
        } else if (action == "upload") {
            string filePath;
            commandStream >> filePath;
            fs::path fullPath = baseDirectory / filePath;

            ofstream file(fullPath, ios::binary);
            if (!file) {
                string response = "Failed to open file for writing.";
                send(clientSocket, response.c_str(), response.length(), 0);
                continue;
            }

            memset(buffer, 0, BUFFER_SIZE);
            int bytes = recv(clientSocket, buffer, BUFFER_SIZE, 0);
            while (bytes > 0) {
                file.write(buffer, bytes);
                memset(buffer, 0, BUFFER_SIZE);
                bytes = recv(clientSocket, buffer, BUFFER_SIZE, 0);
            }

            file.close();
            string response = "File uploaded successfully.";
            send(clientSocket, response.c_str(), response.length(), 0);
        } else if (action == "download") {
            string filePath;
            commandStream >> filePath;
            fs::path fullPath = baseDirectory / filePath;

            if (fs::exists(fullPath) && fs::is_regular_file(fullPath)) {
                ifstream file(fullPath, ios::binary);
                memset(buffer, 0, BUFFER_SIZE);
                while (file.read(buffer, BUFFER_SIZE)) {
                    send(clientSocket, buffer, BUFFER_SIZE, 0);
                    memset(buffer, 0, BUFFER_SIZE);
                }
                send(clientSocket, buffer, file.gcount(), 0);
                file.close();
            } else {
                string response = "File not found.";
                send(clientSocket, response.c_str(), response.length(), 0);
            }
        } else if (action == "delete") {
            string filePath;
            commandStream >> filePath;
            fs::path fullPath = baseDirectory / filePath;

            if (fs::exists(fullPath) && fs::is_regular_file(fullPath)) {
                fs::remove(fullPath);
                string response = "File deleted successfully.";
                send(clientSocket, response.c_str(), response.length(), 0);
            } else {
                string response = "File not found.";
                send(clientSocket, response.c_str(), response.length(), 0);
            }
        } else if (action == "overwrite") {
            string filePath;
            commandStream >> filePath;
            fs::path fullPath = baseDirectory / filePath;

            ofstream file(fullPath, ios::trunc | ios::binary);
            if (!file) {
                string response = "Failed to open file for overwriting.";
                send(clientSocket, response.c_str(), response.length(), 0);
                continue;
            }

            memset(buffer, 0, BUFFER_SIZE);
            int bytes = recv(clientSocket, buffer, BUFFER_SIZE, 0);
            while (bytes > 0) {
                file.write(buffer, bytes);
                memset(buffer, 0, BUFFER_SIZE);
                bytes = recv(clientSocket, buffer, BUFFER_SIZE, 0);
            }

            file.close();
            string response = "File overwritten successfully.";
            send(clientSocket, response.c_str(), response.length(), 0);
        } else {
            string response = "Unknown command";
            send(clientSocket, response.c_str(), response.length(), 0);
        }
    }

    // Remove client from the list when disconnected
    clientMutex.lock();
    clientSockets.erase(remove(clientSockets.begin(), clientSockets.end(), clientSocket), clientSockets.end());
    clientMutex.unlock();
    
    close(clientSocket);
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        cerr << "Usage: " << argv[0] << " -p <port> -d <directory>" << endl;
        return 1;
    }

    int port;
    string directory;

    for (int i = 1; i < argc; i++) {
        if (string(argv[i]) == "-p" && i + 1 < argc) {
            port = stoi(argv[++i]);
        } else if (string(argv[i]) == "-d" && i + 1 < argc) {
            directory = argv[++i];
        } else {
            cerr << "Invalid arguments" << endl;
            return 1;
        }
    }

    fs::path baseDirectory(directory);
    if (!fs::exists(baseDirectory) || !fs::is_directory(baseDirectory)) {
        cerr << "Invalid directory" << endl;
        return 1;
    }

    signal(SIGINT, signalHandler);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        cerr << "Failed to create socket" << endl;
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        cerr << "Failed to bind to port" << endl;
        return 1;
    }

    if (listen(serverSocket, 5) < 0) {
        cerr << "Failed to listen on socket" << endl;
        return 1;
    }

    cout << "Server is running on port " << port << " and serving directory " << directory << endl;

    while (serverRunning) {
        sockaddr_in clientAddress;
        socklen_t clientSize = sizeof(clientAddress);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientSize);
        if (clientSocket < 0) {
            cerr << "Failed to accept client connection" << endl;
            continue;
        }

        cout << "Client connected." << endl;
        clientMutex.lock();
        clientSockets.push_back(clientSocket);
        clientMutex.unlock();

        thread clientThread(handleClient, clientSocket, baseDirectory);
        clientThread.detach();
    }

    close(serverSocket);
    return 0;
}
