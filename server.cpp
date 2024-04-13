#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

int main() {
    // creating socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        cerr << "Error in creating socket" << endl;
        return 1;
    }

    // specifying the address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // binding socket.
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        cerr << "Bind failed" << endl;
        close(serverSocket);
        return 1;
    }

    // listening to the assigned socket
    if (listen(serverSocket, 5) == -1) {
        cerr << "Listen failed" << endl;
        close(serverSocket);
        return 1;
    }

    // accepting connection request
    int clientSocket = accept(serverSocket, nullptr, nullptr);
    if (clientSocket == -1) {
        cerr << "Accept failed" << endl;
        close(serverSocket);
        return 1;
    }

    // receiving data
    char buffer[1024] = {0};
    int bytesReceived;
    while ((bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        cout << buffer << endl;
        // Clearing the buffer for the next receive
        memset(buffer, 0, sizeof(buffer));
    }
    if (bytesReceived == -1) {
        cerr << "Receive failed" << endl;
    }

    // closing the sockets.
    close(clientSocket);
    close(serverSocket);

    return 0;
}
