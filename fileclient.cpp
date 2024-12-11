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
#include <set>
#include <dirent.h>
#include <sys/stat.h>
#include <stack>
#include <filesystem>

#include "clientparse.h"
#include "socket.h"

using namespace std;
namespace fs = std::filesystem;

//recvall a file from the server
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





void getRecursive(mysock &s, const string &remote_path, const string &local_path) {
    // Send the recursive get request to the server
    s.clientsend("get -R " + remote_path);

    char response_buffer[1024];
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







// put function for directories and files
void putRecursive(mysock &s, const string &local_path, const string &remote_path) {
    // Ensure the remote directory exists
    s.clientsend("mkdir " + remote_path);
    char buffer[1024];
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
            char response[1024];
            int receivedBytes = s.clientrecv(response, sizeof(response));
            cout << string(response, receivedBytes) << endl;
        } else if (command == "pwd") {
            s.clientsend("pwd");
            char response[1024];
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
            char response[1024];
            int receivedBytes = s.clientrecv(response, sizeof(response));
            cout << string(response, receivedBytes) << endl;
        } else if (command == "mkdir") {
            s.clientsend("mkdir " + argument);
            char response[1024];
            int receivedBytes = s.clientrecv(response, sizeof(response));
            cout << string(response, receivedBytes) << endl;
        } else if (command == "lmkdir") {
            if (argument.empty()) {
                cout << "Error: Directory name not specified.\n";
                continue;
            }
            // Attempt to create the directory
            if (mkdir(argument.c_str(), 0755) == 0) {
                cout << "Local directory created: " << argument << endl;
            } else {
                perror("Error creating local directory");
        }} else if (command == "get") {
            // Parse paths for the "get" command
            size_t flag_pos = argument.find("-R");
            bool recursive = (flag_pos != string::npos);

            string remote_path = argument;
            string local_path = argument;

            if (recursive) {
                remote_path = argument.substr(flag_pos + 3); // Skip the `-R` flag
            }

            // Split remote and local paths if two arguments are provided
            size_t space_pos = remote_path.find_last_of(' ');
            if (space_pos != string::npos) {
                local_path = remote_path.substr(space_pos + 1);
                remote_path = remote_path.substr(0, space_pos);
            }

            if (remote_path.empty()) {
                cout << "Error: Remote path not specified.\n";
                continue;
            }

            // Distinguish between file and directory
            if (recursive) {
                getRecursive(s, remote_path, local_path);
            } else {
                s.clientsend("get " + remote_path);
                recvallFile(s, local_path);
            }
        } else if (command == "put") {
            size_t flag_pos = argument.find("-R");
            bool recursive = (flag_pos != string::npos);

            string local_path = argument;
            string remote_path = argument;

            if (recursive) {
                local_path = argument.substr(flag_pos + 3); // Skip `-R`
            }

            // Split local and remote paths if two arguments are provided
            size_t space_pos = local_path.find_last_of(' ');
            if (space_pos != string::npos) {
                remote_path = local_path.substr(space_pos + 1);
                local_path = local_path.substr(0, space_pos);
            }

            if (local_path.empty()) {
                cout << "Error: Local path not specified.\n";
                continue;
            }

            // Distinguish between file and directory
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
        } 
    }
    s.close();
    return 0;
}
