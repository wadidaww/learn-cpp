#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h> // Required for inet_pton
#include <netdb.h>

const int PORT = 8080;
const int BUFFER_SIZE = 256;
const char* SERVER_ADDRESS = "127.0.0.1"; //localhost

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error \n";
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, SERVER_ADDRESS, &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported \n";
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed \n";
        return -1;
    }

    // Send messages in a loop
    for (int i = 0; i < 10; ++i) {
        std::string message = "Message " + std::to_string(i);
        send(sock, message.c_str(), message.length(), 0);
        std::cout << "Sent: " << message << std::endl;

        // Receive response
        long valread = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (valread > 0) {
            buffer[valread] = '\0';
            std::cout << "Received: " << buffer << std::endl;
        } else if (valread == 0) {
            std::cout << "Server disconnected" << std::endl;
            break;
        } else {
            perror("recv failed");
            break;
        }

        // Small delay between messages
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Closing the socket
    close(sock);
    return 0;
}