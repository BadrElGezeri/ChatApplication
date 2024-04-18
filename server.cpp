#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <iomanip>

using namespace std;

const int MAX_ROOMS = 10;
const int MAX_CLIENTS_PER_ROOM = 10;
const int MAX_ROOM_NAME_LENGTH = 50;
const int MAX_USERNAME_LENGTH = 50;

struct Room {
    int clients[MAX_CLIENTS_PER_ROOM];
    int numClients;
    char name[MAX_ROOM_NAME_LENGTH];

    Room() : numClients(0) {
        memset(clients, -1, sizeof(clients));
        memset(name, 0, sizeof(name));
    }

    void addClient(int clientSocket) {
        if (numClients < MAX_CLIENTS_PER_ROOM) {
            clients[numClients++] = clientSocket;
        }
    }

    void removeClient(int clientSocket) {
        for (int i = 0; i < numClients; ++i) {
            if (clients[i] == clientSocket) {
                for (int j = i; j < numClients - 1; ++j) {
                    clients[j] = clients[j + 1];
                }
                clients[numClients - 1] = -1; // Mark as disconnected
                numClients--;
                break;
            }
        }
    }

    void broadcastMessage(const char* message, int senderSocket) {
        for (int i = 0; i < numClients; ++i) {
            if (clients[i] != senderSocket) {
                send(clients[i], message, strlen(message), 0);
            }
        }
    }
};

Room rooms[MAX_ROOMS];


// Function to compute the SHA-256 hash of a password
string computeHash(const string& password) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        cerr << "Error creating context" << endl;
        return "";
    }

    const EVP_MD* md = EVP_sha256();

    if (!EVP_DigestInit_ex(ctx, md, nullptr)) {
        cerr << "Error initializing digest" << endl;
        EVP_MD_CTX_free(ctx);
        return "";
    }

    if (!EVP_DigestUpdate(ctx, password.c_str(), password.length())) {
        cerr << "Error updating digest" << endl;
        EVP_MD_CTX_free(ctx);
        return "";
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen;
    if (!EVP_DigestFinal_ex(ctx, hash, &hashLen)) {
        cerr << "Error finalizing digest" << endl;
        EVP_MD_CTX_free(ctx);
        return "";
    }

    EVP_MD_CTX_free(ctx);

    // Convert the binary hash to a hex string
    ostringstream ss;
    ss << hex << setfill('0');
    for (unsigned int i = 0; i < hashLen; ++i) {
        ss << setw(2) << static_cast<unsigned int>(hash[i]);
    }

    return ss.str();
}






void handleClient(int clientSocket, const char* username, const char* roomName) {
    char buffer[1024] = {0};
    int bytesReceived;
    Room* room = nullptr;

    // Find the room
    for (int i = 0; i < MAX_ROOMS; ++i) {
        if (strcmp(rooms[i].name, roomName) == 0) {
            room = &rooms[i];
            break;
        }
    }

    if (!room) {
        cerr << "Room not found: " << roomName << endl;
        close(clientSocket);
        return;
    }

    // Add client to the room
    room->addClient(clientSocket);

    // Notify other clients about new user
    string userJoined = username;
    userJoined += " joined the room.";
    room->broadcastMessage(userJoined.c_str(), clientSocket);

    while ((bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        cout << "[" << roomName << "] " << buffer << endl;
        // Broadcast the message to all other clients in the room
        string message;
        message += buffer;
        room->broadcastMessage(message.c_str(), clientSocket);
        memset(buffer, 0, sizeof(buffer));
    }

    if (bytesReceived == -1) {
        cerr << "Receive failed" << endl;
    }

    // Remove client from the room
    room->removeClient(clientSocket);

    // Notify other clients about user leaving
    string userLeft = username;
    userLeft += " left the room.";
    room->broadcastMessage(userLeft.c_str(), clientSocket);

    close(clientSocket);
}

int main() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        cerr << "Error in creating socket" << endl;
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        cerr << "Bind failed" << endl;
        close(serverSocket);
        return 1;
    }

    if (listen(serverSocket, 5) == -1) {
        cerr << "Listen failed" << endl;
        close(serverSocket);
        return 1;
    }

    while (true) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == -1) {
            cerr << "Accept failed" << endl;
            close(serverSocket);
            return 1;
        }
        

        char username[MAX_USERNAME_LENGTH];
        recv(clientSocket, username, sizeof(username), 0);

        char roomName[MAX_ROOM_NAME_LENGTH];
        recv(clientSocket, roomName, sizeof(roomName), 0);

        bool roomCreated = false;
        Room* room = nullptr;
        for (int i = 0; i < MAX_ROOMS; ++i) {
            if (strlen(rooms[i].name) == 0) {
                strncpy(rooms[i].name, roomName, sizeof(rooms[i].name) - 1);
                room = &rooms[i];
                roomCreated = true;
                break;
            }
        }

        if (!roomCreated) {
            cerr << "No more room available." << endl;
            close(clientSocket);
            continue;
        }

        thread clientThread(handleClient, clientSocket, username, room->name);
        clientThread.detach();
    }

    close(serverSocket);

    return 0;
}
