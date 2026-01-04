// C++ program to show the example of server application in
// socket programming
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

inline const int PORT = 8080;

int main()
{
    // creating socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    // specifying the address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // binding socket.
    if (bind(serverSocket, (struct sockaddr*)&serverAddress,
         sizeof(serverAddress)) < 0) {
        cerr << "Bind failed\n";
        close(serverSocket);
        return 1;
    }

    // listening to the assigned socket
    if (listen(serverSocket, 5) < 0) {
        std::cerr << "Listen failed\n";
        close(serverSocket);
        return 1;
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    // accepting connection request
    int clientSocket
        = accept(serverSocket, nullptr, nullptr);

    int addrlen = sizeof(serverAddress);
    clientSocket = accept(serverSocket, (struct sockaddr*)&serverAddress, (socklen_t*)&addrlen);
    if (clientSocket < 0) {
        std::cerr << "Accept failed\n";
        close(serverSocket);
        return 1;
    }

    // recieving data
    char buffer[1024] = { 0 };
    recv(clientSocket, buffer, sizeof(buffer), 0);
    cout << "Message from client: " << buffer
              << endl;

    // closing the socket.
    close(serverSocket);

    return 0;
}