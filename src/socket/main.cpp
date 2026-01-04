#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    const int PORT = 8080;

    // Create socket file descriptor
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Socket creation failed\n";
        return 1;
    }

    // Bind to port
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed\n";
        close(server_fd);
        return 1;
    }

    // Listen for connections
    if (listen(server_fd, 3) < 0) {
        std::cerr << "Listen failed\n";
        close(server_fd);
        return 1;
    }
    std::cout << "Server listening on port " << PORT << std::endl;

    // Accept a connection
    client_fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    if (client_fd < 0) {
        std::cerr << "Accept failed\n";
        close(server_fd);
        return 1;
    }

    // Send a message to the client
    const char* msg = "Hello from server!";
    send(client_fd, msg, strlen(msg), 0);

    // Close sockets
    close(client_fd);
    close(server_fd);

    return 0;
}