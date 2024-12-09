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

## File/Folder Manifest

- **`fileserver.cpp`**: Implements the server application, including client handling, command parsing, and file operations.
- **`fileclient.cpp`**: Implements the client application with an interactive REPL for sending commands to the server.
- **`socket.cpp` / `socket.h`**: Encapsulates socket operations such as `connect`, `bind`, `listen`, `accept`, and data transmission.
- **`clientparse.cpp` / `clientparse.h`**: Handles command-line argument parsing for the client.
- **Makefile**: Automates the build process.

## Responsibility List

- **Lizmary Delarosa**: Developed client-side functionality, including parsing, and implemented interactive REPL commands.
- **Arkadiusz Gebka**: Designed and implemented server-side functionality, including recursive directory handling and file security validations.

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

3. **Client-Side Crashes During Recursive Uploads and Downloads**:

   - **Problem**: The client occasionally crashes during recursive operations due to memory management or large data transfers.
   - **Solution**: Added buffer size limits and enhanced error handling to detect and address issues during file transmission.

## Status

The project is fully functional and meets the specified requirements. Known issues:

- Error handling for invalid commands can be improved.
- Limited support for non-ASCII filenames.

## Authors

- Lizmary Delarosa and Arkadiusz Gebka

