# Chat Application Documentation
!["App Logo"](image2)

## Overview

This repository contains two main parts of a chat application:

1. **Client Code**: Handles user registration, login, and messaging functionality using a simple Caesar cipher for message encryption.
2. **Server Code**: Manages client connections, broadcasts messages to connected clients, and handles user disconnections.

## Prerequisites

- For the client code:
  - GCC (G++) compiler
  - C++ Standard Library
  - Unix-based system (Linux or macOS) for system calls compatibility
- For the server code:
  - Microsoft Visual Studio or a compatible compiler that supports C++ with Winsock2 API
  - Windows environment

## Installation

### Client

1. Clone the repository to your local machine.
2. Navigate to the client directory.
3. Compile the code using a C++ compiler. For example, using g++:
   ```bash
   g++ -o chatClient client.cpp -lpthread
   ```
4. Run the executable:
   ```bash
   ./chatClient
   ```

### Server

1. Ensure that your Windows system has the Winsock2 API compatibility.
2. Clone the repository to your local machine.
3. Navigate to the server directory.
4. Open the project in Microsoft Visual Studio and build the project, or compile using:
   ```bash
   cl /EHsc /D_WIN32_WINNT=0x0601 server.cpp /link Ws2_32.lib
   ```
5. Run the compiled executable.

## Features

- **Client:**
  - User registration and login with encrypted credentials storage.
  - Real-time message sending and receiving with basic encryption.
  - Terminal-based user interface with timestamps and colored text.

- **Server:**
  - Supports multiple client connections using TCP/IP.
  - Broadcasts messages to all connected clients except the sender.
  - Handles client disconnections and cleans up sockets properly.

## Dependencies

- Client:
  - pthread library for threading support.
- Server:
  - Winsock2 API for Windows socket programming.

## Contributing

Contributions are welcome! For major changes, please open an issue first to discuss what you would like to change.
feedback and cotact: ym2101372@tkh.edu.eg
