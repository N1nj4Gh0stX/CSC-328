# File Server and Client Project

## ID Block

**Team Members:**

- Lizmary Delarosa
- Arkadiusz Gebka

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

## How to Build and Run the Client and Server

### Prerequisites

- C++17 or later
- POSIX-compatible system

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

If used within an academic system, it is recommended to create two dedicated directories: one for the server and one for the local client. These directories should contain pre-created, organized test files. This structure simplifies and accelerates testing by avoiding the need to generate files during runtime.

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

## File/Folder Manifest

- **`fileserver.cpp`**: Implements the server application, including client handling, command parsing, and file operations.
- **`fileclient.cpp`**: Implements the client application with an interactive REPL for sending commands to the server.
- **`socket.cpp` / `socket.h`**: Encapsulates socket operations such as `connect`, `bind`, `listen`, `accept`, and data transmission.
- **`clientparse.cpp` / `clientparse.h`**: Handles command-line argument parsing for the client.
- **Makefile**: Automates the build process.

## Command Testing Reference

### Overview

This section outlines the commands we used during the testing phase of the application. These commands were designed to verify functionality across different aspects of the client-server system, including directory management, file transfer, and cleanup operations.

### I. Basic Testing

- **Display Help Menu**:
  ```
  help
  ```

- **Verify Remote Directory**:
  ```
  pwd
  ```

- **Verify Local Directory**:
  ```
  lpwd
  ```

### II. Remote Directory Management

- **List Remote Directory Contents**:
  ```
  ls
  ```

- **Create a Remote Directory**:
  ```
  mkdir testRemoteDir
  ```

- **Change to the New Remote Directory**:
  ```
  cd testRemoteDir
  ```

- **Verify Remote Directory Change**:
  ```
  pwd
  ```

### III. Local Directory Management

- **List Local Directory Contents**:
  ```
  lls
  ```

- **Create a Local Directory**:
  ```
  lmkdir testLocalDir
  ```

- **Change to the New Local Directory**:
  ```
  lcd testLocalDir
  ```

- **Verify Local Directory Change**:
  ```
  lpwd
  ```

### IV. File Transfer

- **Upload a File to the Remote Directory**:
  ```
  put sample.txt testRemoteDir/sample.txt
  ```

- **Download the File Back to Local Directory**:
  ```
  get testRemoteDir/sample.txt downloaded_sample.txt
  ```

- **Upload a Local Directory Recursively**: (Does not work in current implemenentation)
  ```
  put -R testLocalDir testRemoteDir/remoteTestDir
  ```

- **Download a Remote Directory Recursively**:  (Does not work in current implemenentation)
  ```
  get -R testRemoteDir/remoteTestDir testLocalDownloadedDir
  ```

### V. Cleaning Up

- **Change to Parent Remote Directory**:
  ```
  cd ..
  ```

- **List Remote Directory Contents to Verify Clean-Up**:
  ```
  ls
  ```

- **Exit the Application**:
  ```
  exit
  ```

## Protocol

### Wire Protocol

Commands and responses are serialized as plain text strings over the socket. Each command is structured as:

```plaintext
<command> [arguments]
```

Examples:

- `get /path/to/file`
- `put /path/to/remote/file`
- `ls /remote/directory`

Responses include status messages or file data in plain text, with `EOF` marking the end of transmissions for files or directory listings.

## Assumptions

- The server operates within a predefined base directory and does not allow access to files outside this directory.
- File and directory names are ASCII-encoded.
- Users pre-create directories for testing, ensuring a clear organization of server and client files.
- Client and server are run on compatible systems with the same TCP/IP configurations.

## Development Process

### Key Decisions

- **Recursive File Handling**: Chose to use `std::filesystem` for efficient traversal and handling of directories.
- **Socket Abstraction**: Encapsulated socket operations in a reusable `mysock` class to simplify error handling.

### Challenges

1. **Directory Traversal Security**:

   - **Problem**: Preventing unauthorized access to files outside the base directory.
   - **Solution**: Verified all file paths against the base directory using absolute paths.

2. **Concurrency**:

   - **Problem**: Managing multiple clients simultaneously.
   - **Solution**: Used process forking to isolate each client session.

## Status

The project is fully functional and meets the specified requirements. Known issues:

- Error handling for invalid commands can be improved.
- Limited support for non-ASCII filenames.
- The client occasionally crashes during recursive uploads and downloads of directories. Restarting the client-side program is required to resume operations.
- The server displays the shutdown message twice when terminated using `Ctrl+C`.

