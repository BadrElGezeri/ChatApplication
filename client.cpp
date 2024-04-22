#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>

using namespace std;

string username;
char userChoice;

string encrypt(string text)
{
    string result = "";
    int key = 4; // Encryption Key

    string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890!@#$%^&*()_=-+"; // Encryption Alphabet

    // Traverse text
    for (int i = 0; i < text.length(); i++) {
        
        size_t pos = alphabet.find(text[i]); // Find the position of the current character in the alphabet


        if (pos != string::npos) { // Validation if the character is found within the alphabet
            pos = (pos + key) % alphabet.length(); // Shift the character by the key
            result += alphabet[pos]; // Append the encrypted character to the result
        }
        else {
            result += text[i];// If the character is not found in the alphabet, keep it unchanged
        }
    }
    return result; // Return the resulting string
}

string decrypt(string text)
{
    string result = "";
    int key = 4;
    string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890!@#$%^&*()_=-+"; // String alphabet

    // Traverse text
    for (int i = 0; i < text.length(); i++) {
        // Find the position of the current character in the alphabet
        size_t pos = alphabet.find(text[i]);

        // If the character is found in the alphabet
        if (pos != string::npos) {
 
            pos = (pos - key + alphabet.length()) % alphabet.length(); //  Shift the character by the key in the opposite direction (decrypt)
            
            result += alphabet[pos]; // Append the decrypted character to the result
        }
        else {
            result += text[i]; // If the character is not found in the alphabet, keep it unchanged
        }
    }

    // Return the resulting string
    return result;
}

void receiveMessages(int socket) {
    char message[1024] = {0};
    int bytesReceived;
    while ((bytesReceived = recv(socket, message, sizeof(message), 0)) > 0) {
        //cout << "Encrypted Message: " << message << endl;
        // cout << "Decrypted Message: " << decrypt(message) << endl;
        cout << decrypt(message) << endl;
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
            message = username + " has left the chat room.";
            string encryptedMessage = encrypt(message);
            send(socket, encryptedMessage.c_str(), encryptedMessage.size(), 0);
            cout << "Exiting application" << endl;
            cout << "Press Enter to close" << endl;
            cin.get();
            exit(0);
        }
        message = username + ": " + message;
        string encryptedMessage = encrypt(message);
        send(socket, encryptedMessage.c_str(), encryptedMessage.size(), 0);
    }
}

int main() {
            cout << "[]Pick one of the following options to proceed:" << endl;
            cout << R"(
[1]- Sign Up
[2]- Login
[3]- Exit Application
)";

    cin >> userChoice;
    cin.ignore(); // Ignore the newline character left in the buffer

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

    // Send user choice to server
    send(clientSocket, &userChoice, sizeof(userChoice), 0);

    if (userChoice == '1') {
        cout << "Enter your username: ";
        getline(cin, username);
        cout << "Enter your password: ";
        string password;
        getline(cin, password);
        //cin.ignore();

        string userSignup = username + "|" + password + " ";
        send(clientSocket, userSignup.c_str(), userSignup.size(), 0);
    } else if (userChoice == '2') {
        cout << "Enter your username: ";
        getline(cin, username);
        cout << "Enter your password: ";
        string password;
        getline(cin, password);

        string userLogin = username + "|" + password + " ";
        send(clientSocket, userLogin.c_str(), userLogin.size(), 0);
        char message[1024] = {0};
        int bytesReceived;
        while ((bytesReceived = recv(clientSocket, message, sizeof(message), 0)) > 0) {
        string returnMessage = message;
        cout << returnMessage << endl;
        memset(message, 0, sizeof(message)); // Clear buffer for the next receive
       if(returnMessage == "True"){
        cout << "Enter room name: ";
        string roomName;
        cin >> roomName;
        roomName = roomName + "|";
        send(clientSocket, roomName.c_str(), roomName.size(), 0);
        string JoinedMessage = username + " has joined the room.";
        string encryptedJoinMessage = encrypt(JoinedMessage);
        send(clientSocket, encryptedJoinMessage.c_str(), encryptedJoinMessage.size(), 0);

        thread receiveThread(receiveMessages, clientSocket);
        thread sendThread(sendMessages, clientSocket);

        receiveThread.join();
        sendThread.join();

        close(clientSocket);
        }}
    } else if (userChoice == '3'){
        cout << "Exiting Application...";
        cout.flush();
        sleep(2);
        close(clientSocket);
        exit(1);
        return 1;
    }

    


    return 0;
}
