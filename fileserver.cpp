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
//liz client 
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
#include <dirent.h>
#include <sys/stat.h>
#include <stack>
#include <filesystem>

#include "parse.h"
#include "socket.h"

using namespace std;
namespace fs = std::filesystem;

//recvall a file from the server
void recvallFile(mysocket &s, const string &remote_file, const string &local_file) {
	ofstream outfile(local_file, ios::binary); // open the local file for writing in binary mode
	string file_data = s.recvall(); // receive data from the server
	while (file_data != "EOF") { // continue until EOF is received
		outfile.write(file_data.c_str(), file_data.size()); // write received data to the local file
		file_data = s.recvall(); // receive the next chunk of data
	}
	outfile.close(); // close the local file
	cout << "File recieved: " << local_file << endl;
}

//get function for directories and files
void getRecursive(mysocket &s, const string &remote_path, const string &local_path) {
	s.sendall("get -R " + remote_path);
	string response = s.recvall();

	if (response == "Error") {
		cout << "Error: Directory not found on the server.\n";
		return;
	}

	try {
		fs::create_directory(local_path); // create the local directory
	} 
	catch (const fs::filesystem_error& e) {
		cout << "Error creating local directory: " << e.what() << endl;
		return;
	}

	s.sendall("ls " + remote_path); // send ls command to list the files in the remote directory
	string file_list = s.recvall();  // get the list of files
	stringstream ss(file_list);
	string filepath;
	while (getline(ss, filepath, '\n')) {
		if (filepath.back() == '/') { // check if it's a directory
		    getRecursive(s, remote_path + "/" + filepath, local_path + "/" + filepath);   // recursively get directory
		} 
		else { // its a file instead
		    s.sendall("get " + remote_path + "/" + filepath);
		    recvallFile(s, remote_path + "/" + filepath, local_path + "/" + filepath);
		}
	}
}

//put function for directories and files
void putRecursive(mysocket &s, const string &local_path, const string &remote_path) {
	s.sendall("mkdir " + remote_path); // create the remote directory
	string response = s.recvall();

	if (response == "Error") {
		cout << "Error creating remote directory: " << remote_path << endl;
		return;
	}

	try {
		for (const auto& entry : fs::recursive_directory_iterator(local_path)) {
			string local_file_path = entry.path().string(); // get the local file path
			string relative_path = fs::relative(entry.path(), local_path).string(); // get the relative path for the file
			string remote_file_path = remote_path + "/" + relative_path; // construct the remote file path

			if (entry.is_directory()) { // check if the entry is a directory
				s.sendall("mkdir " + remote_file_path);
				s.recvall(); // Assume server handles mkdir success/failure
			} 
			else { // its a file
				ifstream infile(local_file_path, ios::binary); // opening file
				s.sendall("put " + remote_file_path);
				char buffer[1024];

				while (infile.read(buffer, sizeof(buffer))) { // read data into the buffer
					s.sendall(string(buffer, infile.gcount()));
				}

				infile.close();
				s.sendall("EOF");
				cout << "File uploaded: " << local_file_path << " -> " << remote_file_path << endl;
			}
		}
	} 
	catch (const fs::filesystem_error& e) {
		cout << "Error accessing local directory: " << e.what() << endl;
	}
}
//lwd function
void printLocalWorkingDirectory() {
	char cwd[PATH_MAX]; // PATH_MAX is the maximum length for paths
	if (getcwd(cwd, sizeof(cwd)) != nullptr) {  // get the current working directory
		cout << "Local working directory: " << cwd << endl;
	} 
	else {
		perror("Error getting local working directory");
	}
}
//help function
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


int main(int argc, char** argv) {
	struct options o = parsemenu(argc, argv);

	mysocket s;
	s.connect(o.hostname, o.port);
	cout << "Connected to server." << endl;

	// REPL loop
	while (true) {
	cout << "client> ";
	string input;
	getline(cin, input);

	if (input.empty()) {
		continue;
	}

	size_t len = input.find(' ');
	string command = input.substr(0, len); // extract the command
	string argument;
	if (len != string::npos) {
		argument = input.substr(len + 1); // extract the argument
	}

	if (command == "exit") {
		s.sendall("exit");
		cout << "Exiting...\n";
		break;
	} 
	else if (command == "help") {
		displayHelp(); // display help information
	} 
	else if (command == "lpwd") {
		printLocalWorkingDirectory(); // print local working directory
		cout << "Local Directory Recieved." << endl;
	} 
	else if (command == "cd") {
		s.sendall("cd " + argument);  // change remote directory
		cout << "Server response: " << s.recvall() << endl;
	} 
	else if (command == "pwd") {
		s.sendall("pwd"); // print remote working directory
		cout << "Remote working directory: " << s.recvall() << endl;
	} 
	else if (command == "ls") {
		s.sendall("ls " + argument); // list remote directory contents
		cout << "Remote directory contents:\n" << s.recvall() << endl;
	} 
	else if (command == "mkdir") { // create a remote directory
		if (argument.empty()) {
			cout << "Error: Path not specified for remote mkdir.\n";
			continue;
		}
		s.sendall("mkdir " + argument); // send command to create directory
		string response = s.recvall();  // get server response
		cout << "Server response: " << response << endl;
	}
	else if (command == "lmkdir") { // create a local directory
		if (argument.empty()) {
			cout << "Error: Path not specified for local mkdir.\n";
			continue;
		}
		if (fs::create_directory(argument)) { // Create directory locally
			cout << "Local directory created: " << argument << endl;
		} 
		else {
			cout << "Error: Failed to create local directory.\n";
		}
	} 
	else if (command == "get") {
		size_t flag_pos = argument.find("-R");  // check if -R flag is present
		bool recursive = (flag_pos != string::npos);  // set recursive flag
		string remote_path;
		string local_path;

		if (recursive) {
			remote_path = argument.substr(flag_pos + 3);  // extract remote path after -R flag
		} 
		else {
			
			remote_path = argument;
		}

		size_t local_pos = remote_path.find_last_of(' '); // find last space for local path
		if (local_pos != string::npos) {
			local_path = remote_path.substr(local_pos + 1); // extract local path
			remote_path = remote_path.substr(0, local_pos); // update remote path
		}
		}

		if (remote_path.empty()) {
			cout << "Error: Remote path not specified.\n";
			continue;
		}

		if (local_path.empty()) {
			local_path = remote_path; // using the remote path as local path if not specified
		}

		getRecursive(s, remote_path, local_path);
	} 
	else if (command == "lcd") {
		if (argument.empty()) {
			argument = getenv("HOME"); // use home directory if no argument is provided
		}

		if (chdir(argument.c_str()) == 0) {
			cout << "Local directory changed to: " << argument << endl;
		} 
		else {
			perror("Error changing local directory");
		}
	} 
	else if (command == "lls") {
		string path;
		if (argument.empty()) {
			path = "."; // default to current directory
		} 
		else {
		path = argument;
		}

		try {
			for (const auto& entry : fs::directory_iterator(path)) {
				cout << entry.path().filename() << endl; // list local directory content
			}
		} 
		catch (const fs::filesystem_error& e) {
			cerr << "Error opening directory: " << e.what() << endl;
		}
	} 
	else if (command == "put") {
		
		size_t flag_pos = argument.find("-R"); // check if -R flag is present
		bool recursive = (flag_pos != string::npos); // set recursive flag
		string local_path;
		string remote_path;

		if (recursive) { // if recursive flag is present
			local_path = argument.substr(flag_pos + 3);
		} 
		else {
			local_path = argument; // otherwise, use the whole argument as the path
		}

		size_t remote_pos = local_path.find_last_of(' '); // find the remote path
		if (remote_pos != string::npos) {
			remote_path = local_path.substr(remote_pos + 1);
			local_path = local_path.substr(0, remote_pos);
		}

		if (local_path.empty()) {
			cout << "Error: Local path not specified.\n";
			continue;
		}

		if (remote_path.empty()) {
			remote_path = local_path; // use the local path as remote path if not specified
		}

		putRecursive(s, local_path, remote_path);
		}
	}
	s.close();

	return 0;
}
//liz socket.cpp
#include <iostream>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sstream>

#include "socket.h"

using namespace std;

mysocket::mysocket(){
	if ((this->fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        stringstream s;
        s << "socket: " << strerror(errno) << endl;
        throw runtime_error(s.str());
}
}

void mysocket::close(){
	::close(fd);
}

void mysocket::connect(string host, string port){
	struct addrinfo hints, *res;
	int retval;
	
	memset(&hints, 0 , sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	
	if ((retval = getaddrinfo(host.c_str(), port.c_str(), &hints, &res)) != 0){
		stringstream s;
		s<< "getaddrinfo: " << gai_strerror(retval) << endl;
		throw runtime_error(s.str());
	}
	if (::connect(this->fd, res->ai_addr, res->ai_addrlen) == -1){
		stringstream s;
		s << "connect: " << strerror(errno) << endl;
		freeaddrinfo(res);
		throw runtime_error(s.str());
	}
	
	freeaddrinfo(res);
}

void mysocket::sendall(string message){
	const char* buf = message.c_str();
	int len = strlen(buf);
	int total = 0;
	int bytesleft = len;
	int n;
	
	while(total < len){
		n = send(this->fd, buf+total, bytesleft, 0);
		if (n == -1) { break; }
		total += n;
		bytesleft -= n;
	}
	
	if (n == -1){
		stringstream s;
		s << "send: " << strerror(errno) << endl;
		throw runtime_error(s.str());
	}
}

string mysocket::recvall(){
	const size_t chunk_size = 4096;
	char buf[chunk_size];
	stringstream result;
	int len;
	
	while ((len = recv(this->fd, buf, chunk_size - 1, 0)) != 0){
		if (len == -1) { break; }
		buf[len] = '\0';
		result << string(buf);
	}
	
	if (len == -1){
		stringstream s;
		s << "recv: " << strerror(errno) << endl;
		throw runtime_error(s.str());
	}
	
	return result.str();
}

//liz parsing.cpp?
#include <iostream>
#include <unistd.h>
#include "parse.h"


using namespace std;

struct options parsemenu(int argc, char* argv[]){
	struct options o;
	o.hostname = "";
	o.port = "";
	int opt;
	while((opt = getopt(argc, argv, "h:p:")) != -1){
		switch (opt){
			case 'h':
				o.hostname = optarg;
				// cout << "hostname: " << o.hostname << endl;
				break;
			
			case 'p':
				o.port = optarg;
				// cout << "port: " << o.port << endl;
				break;
			
		}
	}
	return o;
}



