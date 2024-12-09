# CSC 328 - File Server and Client Project

## Authors
- Lizmary Delarosa and Arkadiusz Gebka

## Overview
This project implements a file server and client application in C++ for transferring files between a server and a client using TCP sockets. The server can serve files from a specific directory, and the client can upload or download files, including support for recursive directory operations. The project adheres to secure coding practices, ensuring that files are only accessed within allowed directories.

## Features
- **Server**
  - Serves files from a specified base directory.
  - Supports recursive directory uploads and downloads.
  - Handles multiple clients using process forking.
  - Implements commands like `ls`, `cd`, `pwd`, and file transfer commands.
  - Validates file access to ensure security.

- **Client**
  - Allows uploading and downloading files.
  - Supports recursive file and directory operations.
  - Interactive command-line interface with commands for navigation and file management.
  - Provides help text for command usage.

## Getting Started
### Prerequisites
- C++17 or later
- POSIX-compatible system
- If used within an academic system, it is recommended to create two dedicated directories: one for the server and one for the local client. This setup makes testing faster and easier, as you can pre-create and organize test files instead of generating them during runtime.

### Build Instructions
Use the provided Makefile to compile the project:

```bash
make
```
This will generate two executables:
- `fileserver`: The server application.
- `fileclient`: The client application.

### Running the Server
To start the server, use:

```bash
./fileserver -p <port> -d <directory>
```
- `<port>`: Port number on which the server listens.
- `<directory>`: Base directory for serving files.

Example:
```bash
./fileserver -p 8080 -d /home/user/shared
```

### Running the Client
To start the client, use:

```bash
./fileclient -h <hostname> -p <port>
```
- `<hostname>`: Server hostname or IP address.
- `<port>`: Port number to connect to.

Example:
```bash
./fileclient -h localhost -p 8080
```

### Directory Structure
If used within an academic system, it is recommended to create two dedicated directories: one for the server and one for the local client. This setup makes testing faster and easier, as you can pre-create and organize test files instead of generating them during runtime.

### Client Commands
| Command                | Description                                                        |
|------------------------|--------------------------------------------------------------------|
| `help`                | Displays a list of available commands.                            |
| `exit`                | Disconnects from the server and exits.                            |
| `cd <path>`           | Changes the remote working directory.                             |
| `lcd <path>`          | Changes the local working directory.                              |
| `pwd`                 | Displays the remote working directory.                            |
| `lpwd`                | Displays the local working directory.                             |
| `ls [path]`           | Lists contents of the remote directory.                           |
| `lls [path]`          | Lists contents of the local directory.                            |
| `mkdir <path>`        | Creates a directory on the server.                                |
| `lmkdir <path>`       | Creates a directory locally.                                      |
| `get <remote> [local]`| Downloads a file or directory from the server to the local system. |
| `put <local> [remote]`| Uploads a file or directory to the server.                         |

## File Descriptions
- **`fileserver.cpp`**: Implements the server application, including client handling, command parsing, and file operations.
- **`fileclient.cpp`**: Implements the client application with an interactive REPL for sending commands to the server.
- **`socket.cpp` / `socket.h`**: Encapsulates socket operations such as `connect`, `bind`, `listen`, `accept`, and data transmission.
- **`clientparse.cpp` / `clientparse.h`**: Handles command-line argument parsing for the client.
- **Makefile**: Automates the build process.

## Security Features
- Ensures the server only serves files within the specified base directory.
- Handles errors gracefully, providing meaningful feedback to the user.
- Validates user inputs to prevent directory traversal attacks.

