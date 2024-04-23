#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <fstream>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <iomanip>

using namespace std;
string username;
const int MAX_ROOMS = 10;
const int MAX_CLIENTS_PER_ROOM = 10;
const int MAX_ROOM_NAME_LENGTH = 50;
const int MAX_USERNAME_LENGTH = 50;
const int MAX_PASSWORD_LENGTH = 32;
void userSignUp(int &clientSocket), userLogin(int &clientSocket);

struct Room {
    // Variable decleration
    int clients[MAX_CLIENTS_PER_ROOM];
    int numClients;
    char name[MAX_ROOM_NAME_LENGTH];

    Room() : numClients(0) {
        memset(clients, -1, sizeof(clients));
        memset(name, 0, sizeof(name));
    }

    void addClient(int clientSocket) {
        if (numClients < MAX_CLIENTS_PER_ROOM) { // If number of clients in the room is less than max clients per room
            clients[numClients++] = clientSocket; // Add the client
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

    void broadcastMessage(const char* message, int senderSocket) { // Send message to all users in the room
        for (int i = 0; i < numClients; ++i) {
            if (clients[i] != senderSocket) {
                send(clients[i], message, strlen(message), 0);
            }
        }
    }
};

Room rooms[MAX_ROOMS];


// Hash calculation using envelopes openssl 
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


stringstream time(){
    auto now = chrono::system_clock::now();
    auto UTC = chrono::duration_cast<chrono::seconds>(now.time_since_epoch()).count();

    auto in_time_t = chrono::system_clock::to_time_t(now);
    stringstream datetime;
    datetime << put_time(localtime(&in_time_t), "%Y-%m-%d %X");
    return datetime;
}


void handleClient(int clientSocket) {
    // Receive user choice and place it into char array
    char userChoice[1];
    recv(clientSocket, userChoice, sizeof(userChoice), 0);

    string userChoiceSTR = userChoice;
    int userChoiceINT = stoi(userChoiceSTR);

    Room* room = nullptr; // Declare room variable here


    // Handle each case of user choice
    if (userChoiceINT == 1) {
        userSignUp(clientSocket);
    } else if (userChoiceINT == 2) {
        userLogin(clientSocket);
    }
    
    // Receiving room name and creating room
    char roomName[MAX_ROOM_NAME_LENGTH];
    memset(roomName, '\0', sizeof(roomName));
    recv(clientSocket, roomName, sizeof(roomName), 0);
    stringstream ss(roomName);
    string roomNameClean;
    char roomArrayName[roomNameClean.length() + 1];
    strcpy(roomArrayName, roomNameClean.c_str());
    getline(ss, roomNameClean, '|');
    bool roomCreated = false;
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
    }

    char buffer[1024] = {0};
    int bytesReceived;

    // Find the room
    for (int i = 0; i < MAX_ROOMS; ++i) {
        if (strcmp(rooms[i].name, roomName) == 0) {
            room = &rooms[i];
            break;
        }
    }

    // Add client to the room
    room->addClient(clientSocket);
    stringstream datetime = time();

    while ((bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        cout << "[" << roomName << "] " << buffer << endl;
        // Broadcast the message to all other clients in the room
        string message;
        message += buffer;
        // Save Log of messages
        ofstream usersFile("log.txt", ios::app);
        if (usersFile.is_open()) {
            //cout << "Hashed Password=" << hashedPassword << endl;
            //cout.flush();
            usersFile <<  "[" << datetime.str() <<"]"<< message << "\n";
            usersFile.close();
        } else {
            cerr << "Error opening users.txt" << endl;
        }

        // Send message to all the users in the same room
        room->broadcastMessage(message.c_str(), clientSocket);
        memset(buffer, 0, sizeof(buffer));
    }

    if (bytesReceived == -1) {
        cerr << "Receive failed" << endl;
    }

    // Remove client from the room
    room->removeClient(clientSocket);

    close(clientSocket);
}


bool isUsernameValid(string username){
    bool usernameValid;
    ifstream usersFile("users.txt");
    string line;
    string loginSuccess = "False";

    // Search through users.txt if the username was used before
    while (getline(usersFile, line)) {
        string usernameFromFile;
        stringstream ss(line);
        getline(ss, usernameFromFile, '|');
        if (username == usernameFromFile){
            usernameValid = false;
            break;
        } else {
            usernameValid = true;
        }
    }

    return usernameValid;
}


void userSignUp(int& clientSocket) {
    char userData[MAX_USERNAME_LENGTH + MAX_PASSWORD_LENGTH + 3]; // Add space for "|" and space separator and null terminator
    recv(clientSocket, userData, sizeof(userData), 0);

    // Extract username and password by splitting at the "|" character
    stringstream ss(userData);
    string usernameSignUp, passwordSignUp;
    getline(ss, usernameSignUp, '|');
    getline(ss, passwordSignUp, ' ');
    if(isUsernameValid(usernameSignUp)){
    cout << "User created with username: " << usernameSignUp << endl;
        // Process username and password

        // Save username and hashed password to users.txt
        ofstream usersFile("users.txt", ios::app);
        if (usersFile.is_open()) {
            //cout << "\nPassword=" << password << endl;
            string hashedPassword = computeHash(passwordSignUp);
            //cout << "Hashed Password=" << hashedPassword << endl;
            //cout.flush();
            usersFile << usernameSignUp << "|" << hashedPassword << "\n";
            usersFile.close();
            string signupSuccessful = "User created successfully!\n";
            send(clientSocket, signupSuccessful.c_str(), signupSuccessful.size(), 0);

        } else {
            cerr << "Error opening users.txt" << endl;
        }
    } else {
        string signupSuccessful = "Username already used!\n";
        send(clientSocket, signupSuccessful.c_str(), signupSuccessful.size(), 0);
    }

}

void userLogin(int &clientSocket){
    char userData[MAX_USERNAME_LENGTH + MAX_PASSWORD_LENGTH + 3]; // Add space for "|" and space separator and null terminator
    recv(clientSocket, userData, sizeof(userData), 0);

    // Extract username and password by splitting at the "|" character
    stringstream ss(userData);
    string usernameLogin, passwordLogin;
    getline(ss, usernameLogin, '|');
    getline(ss, passwordLogin, ' ');

    cout << "Login attempt for user: " << usernameLogin << endl;
        // Validate username and password from users.txt
        ifstream usersFile("users.txt");
        string line;
        string loginSuccess = "False";
        while (getline(usersFile, line)) {
            string usernameFromFile, hashedPassword;
            stringstream ss(line);
            getline(ss, usernameFromFile, '|');
            getline(ss, hashedPassword);

            // Compare username and hashed password
            if (usernameFromFile == usernameLogin) {
                if (hashedPassword == computeHash(passwordLogin)){
                loginSuccess = "Login Successful!";
                cout << "Login Successful" << endl;
                cout.flush();
                send(clientSocket, loginSuccess.c_str(), loginSuccess.size(), 0);
                break;
                }
            }       
        }
        if(loginSuccess != "Login Successful!"){
        loginSuccess = "Username or password incorrect!";
        cout << "Login Unsuccessful" << endl;
        cout.flush();
        send(clientSocket, loginSuccess.c_str(), loginSuccess.size(), 0);}
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
        // Thread creation for handleClient function
        thread clientThread(handleClient, clientSocket);
        clientThread.detach();
    }

    close(serverSocket);

    return 0;
}
