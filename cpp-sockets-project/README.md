# C++ Sockets Project

This project implements a simple TCP server using sockets in C++. The server listens for incoming connections on a specified port and sends a greeting message to the client upon connection.

## Project Structure

```
cpp-sockets-project
├── src
│   └── socket
│       └── main.cpp       # Main implementation of the TCP server
├── CMakeLists.txt         # CMake configuration file
└── README.md              # Project documentation
```

## Requirements

- C++11 or later
- CMake

## Building the Project

1. Navigate to the project directory:

   ```bash
   cd cpp-sockets-project
   ```

2. Create a build directory and navigate into it:

   ```bash
   mkdir build
   cd build
   ```

3. Run CMake to configure the project:

   ```bash
   cmake ..
   ```

4. Build the project:

   ```bash
   make
   ```

## Running the Server

After building the project, you can run the server using the following command:

```bash
./src/socket/main
```

The server will start listening on port 8080. You can connect to it using a TCP client to receive the greeting message.