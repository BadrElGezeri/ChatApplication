#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>

using namespace std;

string username;

void receiveMessages(int socket) {
    char message[1024] = {0};
    int bytesReceived;
    while ((bytesReceived = recv(socket, message, sizeof(message), 0)) > 0) {
        cout << message << endl;
        memset(message, 0, sizeof(message)); // Clear buffer for the next receive
    }
    if (bytesReceived == -1) {
        cerr << "Receive failed" << endl;
    }
}

void sendMessages(int socket) {
    string message;
    while (true) {
        cout << "> ";
        getline(cin, message);
        if (message == "STOP") {
            cout << "Exiting application" << endl;
            cout << "Press Enter to close" << endl;
            cin.get();
            exit(0);
        }
        message = username + ": " + message;
        send(socket, message.c_str(), message.size(), 0);
    }
}

int main() {
    cout << "Enter your username: ";
    getline(cin, username);

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        cerr << "Error in creating socket" << endl;
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);

    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        cerr << "Connection failed" << endl;
        close(clientSocket);
        return 1;
    }

    // Send username to server
    send(clientSocket, username.c_str(), username.size(), 0);

    cout << "Enter room name: ";
    string roomName;
    getline(cin, roomName);
    send(clientSocket, roomName.c_str(), roomName.size(), 0);

    thread receiveThread(receiveMessages, clientSocket);
    thread sendThread(sendMessages, clientSocket);

    receiveThread.join();
    sendThread.join();

    close(clientSocket);

    return 0;
}
