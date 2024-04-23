#include <iostream>
#include <cstring>
#include <fstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <regex>
#include <chrono>
#include <iomanip> 
#include <fstream>
#include <sstream> 

using namespace std;

string username;
int clientSocket;
char userChoice;

void moveCursorUp() {
    cout << "\033[F"; // Move cursor up one line
}

void moveCursorDown() {
    cout << "\033[E"; // Move cursor down one line
}

void clearCurrentLine() {
    cout << "\033[K"; // Clear from cursor position to the end of the line
}
void displayOutput(const string& output) {
    moveCursorUp();     // Move cursor up one line
    clearCurrentLine(); // Clear current line
    cout << output << endl; // Display the output
}

    


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

    auto now = chrono::system_clock::now();
    auto UTC = chrono::duration_cast<chrono::seconds>(now.time_since_epoch()).count();

    auto in_time_t = chrono::system_clock::to_time_t(now);
    stringstream datetime;
    datetime << put_time(localtime(&in_time_t), "%Y-%m-%d %X");

    char message[1024] = {0};
    int bytesReceived;
    while ((bytesReceived = recv(socket, message, sizeof(message), 0)) > 0) {
        //cout << "Encrypted Message: " << message << endl;
        // cout << "Decrypted Message: " << decrypt(message) << endl;
        cout << "\r\033[36m"<< decrypt(message) << "\n\033[36m";
        string fileName = username + "_log.txt";
        ofstream usersFile(fileName, ios::app);
        if (usersFile.is_open()) {
            //cout << "Hashed Password=" << hashedPassword << endl;
            //cout.flush();
            usersFile << "[" << datetime.str() << "]" << decrypt(message) << "\n";
            usersFile.close();
        } else {
            cerr << "Error opening users.txt" << endl;
        }
        displayOutput(decrypt(message));
        memset(message, 0, sizeof(message)); // Clear buffer for the next receive
    }
    if (bytesReceived == -1) {
        cerr << "Receive failed" << endl;
    }
}

void sendMessages(int socket) {
    string message;
    while (true) {
        cout << "\033[97m> \033[32m";
        getline(cin, message);
        clearCurrentLine();

        auto now = chrono::system_clock::now();
        auto UTC = chrono::duration_cast<chrono::seconds>(now.time_since_epoch()).count();

        auto in_time_t = chrono::system_clock::to_time_t(now);
        stringstream datetime;
        datetime << put_time(localtime(&in_time_t), "%Y-%m-%d %X");

        if (message == "STOP") {
            message = username + " has left the chat room.\n";
            string encryptedMessage = encrypt(message);
            send(socket, encryptedMessage.c_str(), encryptedMessage.size(), 0);
            cout << "Exiting application" << endl;
            cout << "Press Enter to close" << endl;
            cin.get();
            exit(0);
        }
        string output = username;
        output += ": " + message;
        string fileName = username + "_log.txt";
        ofstream usersFile(fileName, ios::app);
        if (usersFile.is_open()) {
            //cout << "Hashed Password=" << hashedPassword << endl;
            //cout.flush();
            usersFile << "[" << datetime.str() << "]"<< username << ": "<< message << "\n";
            usersFile.close();
        } else {
            cerr << "Error opening users.txt" << endl;
        }
        moveCursorUp();
        cout << "\033[32m"<< "You: " << message;
        moveCursorDown();
        string encryptedMessage = encrypt(output);
        send(socket, encryptedMessage.c_str(), encryptedMessage.size(), 0);
    }
}

bool isValidUsername(const string& username) {
    // Regular expression pattern for a valid username
    regex pattern("^[a-zA-Z0-9_-]{3,16}$");
    return regex_match(username, pattern);
}





int startupScreen(){
            cout << "[]Pick one of the following options to proceed:" << endl;
            cout << R"(
[1]- Sign Up
[2]- Login
[3]- Exit Application
)";

    cin >> userChoice;
    cin.ignore(); // Ignore the newline character left in the buffer


    if (userChoice == '1') {

        int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == -1) {
            cerr << "Error in creating socket" << endl;
        }

        sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(8080);
        inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);

        if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
            cerr << "Connection failed" << endl;
            close(clientSocket);
        }
        
        send(clientSocket, &userChoice, sizeof(userChoice), 0);   // Send user choice to server
        cout << "Enter your username: ";
        getline(cin, username);
        cout << "Enter your password: ";
        string password;
        getline(cin, password);
        //cin.ignore();

        string userSignup = username + "|" + password + " ";
        send(clientSocket, userSignup.c_str(), userSignup.size(), 0);
    } else if (userChoice == '2') {

        int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == -1) {
            cerr << "Error in creating socket" << endl;
        }

        sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(8080);
        inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);

        if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
            cerr << "Connection failed" << endl;
            close(clientSocket);
        }
    
        send(clientSocket, &userChoice, sizeof(userChoice), 0);
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
       if(returnMessage == "Login Successful!"){
        cout << "\nEnter room name: ";
        string roomName;
        cin >> roomName;
        cout << endl;
        roomName = roomName + "|";
        send(clientSocket, roomName.c_str(), roomName.size(), 0);
        string JoinedMessage = username + " has joined the room.\n";
        string encryptedJoinMessage = encrypt(JoinedMessage);
        send(clientSocket, encryptedJoinMessage.c_str(), encryptedJoinMessage.size(), 0);

        thread receiveThread(receiveMessages, clientSocket);
        thread sendThread(sendMessages, clientSocket);

        receiveThread.join();
        sendThread.join();

        close(clientSocket);
        }else{
            //cout << "Incorrect Username or Password!";
            close(clientSocket);
        }
        }
    } else if (userChoice == '3'){
        cout << "Exiting Application...";
        cout.flush();
        sleep(2);
        //close(clientSocket);
        exit(1);
        return 0;
    } else{
        cout << "[] Invalid input, try again:" << endl;
        startupScreen();
    }

    
    return 0;

}

int main() {

    startupScreen();



    return 0;
}
