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
