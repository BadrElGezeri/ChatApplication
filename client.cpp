#include <iostream>
#include <string>
#include <cstring>
#include <netinet/in.h>
#include <limits>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

int main() {
    // Creating socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        cerr << "Error in creating socket" << endl;
        return 1;
    }

    // Specifying address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Sending connection request
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        cerr << "Connection failed" << endl;
        close(clientSocket);
        return 1;
    }

    int userChoice;
    char username[50]; // Assuming username won't exceed 99 characters
    string message;     // Using std::string for message

    // Sending data
    cout << "Welcome to SecureChat" << endl;
    cout << "Pick one of the following options to proceed" << endl;
    cout << R"(
[1]- Connect to a chat room
[2]- Exit
>)";
    cin >> userChoice;

    if (userChoice == 1) {
        cout << "[]Enter a username:\n>";
        cin >> username;

        // Clearing the input buffer
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Connected to chat room, to exit type \"STOP\"" << endl;
        cout << username << ": ";
        getline(cin, message);
        while(message != "STOP"){
            // Concatenating username and message using std::string
            string output = username;
            output += ": " + message;

            // Sending the concatenated string
            send(clientSocket, output.c_str(), output.length(), 0);
            cout << username << ": ";
            getline(cin, message);
        }
        exit(0);

    } else if (userChoice == 2) {
        exit(0);
    }

    // Closing socket
    close(clientSocket);

    return 0;
}
