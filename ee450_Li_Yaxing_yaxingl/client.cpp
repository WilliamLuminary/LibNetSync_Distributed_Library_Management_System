//
// Created by Yaxing Li on 11/2/23.
//

// Client.cpp
#include "client.h"


#include <iostream>
#include <string>
#include <stdexcept>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <limits>
#include <utility>


using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::pair;


int main() {
    client cl;
    cl.run();
    return 0;
}

void client::run() {
    if (!setup()) {
        cerr << "Setup failed." << endl;
        return;
    }

    if (!connectToServer()) {
        cerr << "Could not connect to the server." << endl;
        return;
    }

    if (!authenticateAndHandleCommunication()) {
        cerr << "Authentication failed." << endl;
        return;
    }

    handleAuthenticatedTcpCommunication();
    closeSocket();
}

bool client::setup() {
    cout << "Client is up and running." << endl;
    socket_fd = createTcpSocket();
    return socket_fd != -1;
}

int client::createTcpSocket() {
    int cfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (cfd == -1) {
        cerr << "Initial Socket failed: " << strerror(errno) << endl;
    }
    return cfd;
}

bool client::connectToServer() {
//    sockaddr_in sock_addr_in{};
    sockaddr_in sock_addr_in;
    memset(&sock_addr_in, 0, sizeof(sock_addr_in));

    sock_addr_in.sin_family = AF_INET;
    sock_addr_in.sin_port = htons(SERVER_M_TCP_PORT);
    if (inet_pton(AF_INET, LOCALHOST_IP, &sock_addr_in.sin_addr) <= 0) {
        cerr << "Invalid IP address format." << endl;
        return false;
    }
    if (connect(socket_fd, reinterpret_cast<struct sockaddr *>(&sock_addr_in), sizeof(sock_addr_in)) == -1) {
        cerr << "Connect failed: " << strerror(errno) << endl;
        return false;
    }
    return true;
}

pair<string, string> client::promptForCredentials() {
    string username, password;
    cout << "Please enter the username: ";
    cin >> username;
    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    cout << "Please enter the password: ";
    cin >> password;
    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return {username, password};
}

void client::sendCredentials(const pair<string, string> &credentials) {
    string encryptedUsername = encrypt(credentials.first);
    string encryptedPassword = encrypt(credentials.second);

    string combinedCredentials = encryptedUsername + "\n" + encryptedPassword;
    ssize_t sent = send(socket_fd, combinedCredentials.c_str(), combinedCredentials.length(), 0);
    if (sent == -1) {
        cerr << "Failed to send credentials: " << strerror(errno) << endl;
        return;
    } else if (sent < static_cast<ssize_t>(combinedCredentials.length())) {
        cerr << "Not all credentials were sent." << endl;
        return;
    }
    cout << credentials.first << " sent an authentication request to the Main Server." << endl;
}

bool client::authenticateAndHandleCommunication() {
    bool isAuthenticated = false;

    while (!isAuthenticated) {
        auto credentials = promptForCredentials();
        userName = credentials.first;
        sendCredentials(credentials);
        isAuthenticated = receiveAuthenticationResult(userName);
    }

    return true;
}

bool client::receiveAuthenticationResult(const string &username) {
    char buffer[BUFFER_SIZE] = {0};
//    sockaddr_in addr{};
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t addr_len = sizeof(addr);
    if (getsockname(socket_fd, (struct sockaddr *) &addr, &addr_len) == -1) {
        cerr << "Error getting peer name: " << strerror(errno) << endl;
        return false;
    }
    int serverPort = ntohs(addr.sin_port);

    ssize_t len = recv(socket_fd, buffer, sizeof(buffer), 0);
    if (len > 0) {
        string response(buffer, len);

        if (response == "Login successful.") {
            cout << username << " received the result of authentication from Main Server using TCP over port "
                 << serverPort << ". Authentication is successful." << endl;
            return true;
        } else if (response.find("is not registered") != string::npos) {
            cout << username << " received the result of authentication from Main Server using TCP over port "
                 << serverPort << ". Authentication failed: Username not found." << endl;
        } else if (response.find("does not match the username") != string::npos) {
            cout << username << " received the result of authentication from Main Server using TCP over port "
                 << serverPort << ". Authentication failed: Password does not match." << endl;
        } else {
            cout << username << " received an unrecognized response from the Main Server: " << response << endl;
        }
    } else if (len == 0) {
        cerr << "Main server has disconnected." << endl;
    } else {
        cerr << "Receiving failed: " << strerror(errno) << endl;
    }
    return false;
}


void client::handleAuthenticatedTcpCommunication() {
    string bookCode;
    bool isAdmin = (userName == "Admin" || userName == "admin"); // You can use a better check here

    while (true) {
        cout << "\n—- Start a new query —-" << endl;
        cout << "Please enter book code to query: ";
        cin >> bookCode;
        cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (!sendBookCodeRequest(bookCode)) {
            cerr << "Failed to send book code to the Main Server." << endl;
            continue;
        }

        if (isAdmin)
            cout << "Request sent to the Main Server with Admin rights." << endl;
        else cout << userName << " sent the request to the Main Server." << endl;

        string serverResponse = receiveServerResponse();
        if (!serverResponse.empty()) {
            int portNum = getHostPort(socket_fd);
            cout << "Response received from the Main Server on TCP port: " << portNum << "." << endl;

            if (isAdmin) {
                parseAdminResponse(serverResponse, bookCode);
            } else {
                parseNonAdminResponse(serverResponse, bookCode);
            }
        } else {
            cerr << "No response received from Main Server." << endl;
        }
    }
}

void client::handleServerErrorResponse(const string &serverResponse, const string &bookCode) {
    if (serverResponse.find("Not able to find") != string::npos) {
        cout << "Not able to find the book-code " << bookCode << " in the system." << endl;
    } else {
        cout << "Response from Main Server: " << serverResponse << endl;
    }
}

void client::parseAdminResponse(const string &serverResponse, const string &bookCode) {
    const string prefix = "INVENTORY:";
    size_t prefixPos = serverResponse.find(prefix);
    string trimmedResponse = (prefixPos != string::npos) ? serverResponse.substr(prefixPos + prefix.length())
                                                         : serverResponse;

    auto delimiterPos = trimmedResponse.find(',');
    if (delimiterPos != string::npos) {
        string code = trimmedResponse.substr(0, delimiterPos);
        int count = std::stoi(trimmedResponse.substr(delimiterPos + 1));
        cout << "Total number of book " << code << " available = " << count << endl;
    } else {
        handleServerErrorResponse(trimmedResponse, bookCode);
    }
}

void client::parseNonAdminResponse(const string &serverResponse, const string &bookCode) {
    if (serverResponse.find("not available") != string::npos) {
        cout << "The requested book " << bookCode << " is NOT available in the library." << endl;
    } else if (serverResponse.find("available") != string::npos) {
        cout << "The requested book " << bookCode << " is available in the library." << endl;
    } else {
        handleServerErrorResponse(serverResponse, bookCode);
    }
}

bool client::sendBookCodeRequest(const string &bookCode) {
    ssize_t sent = send(socket_fd, bookCode.c_str(), bookCode.length(), 0);
    if (sent == -1) {
        cerr << "Failed to send book code: " << strerror(errno) << endl;
        return false;
    } else if (sent < static_cast<ssize_t>(bookCode.length())) {
        cerr << "Not all book code data was sent." << endl;
        return false;
    }
//    cout << userName << " sent the request to the Main Server." << endl;
    return true;
}

string client::receiveServerResponse() {
    char buffer[BUFFER_SIZE] = {0};
    ssize_t len = recv(socket_fd, buffer, sizeof(buffer), 0);
    string response;
    if (len > 0) {
        response = string(buffer, len);
    } else if (len == 0) {
        cerr << "Main Server has disconnected." << endl;
    } else {
        cerr << "Receiving response failed: " << strerror(errno) << endl;
    }
    return response;
}


void client::closeSocket() {
    if (socket_fd != -1) {
        close(socket_fd);
        socket_fd = -1;
        cout << "Socket closed." << endl;
    }
}

string client::encrypt(const string &input) {
    string encrypted = input;
    for (char &c: encrypted) {
        if (isalpha(c)) {
            int offset = (isupper(c) ? 'A' : 'a');
            c = static_cast<char>(((c + 5 - offset) % 26) + offset);
        } else if (isdigit(c)) {
            c = static_cast<char>(((c - '0' + 5) % 10) + '0');
        }
    }
    return encrypted;
}

int client::getHostPort(int socket_fd) {
//    sockaddr_in sAddr{};
    sockaddr_in sAddr;
    memset(&sAddr, 0, sizeof(sAddr));
    socklen_t addrLen = sizeof(sAddr);
    if (getsockname(socket_fd, (struct sockaddr *) &sAddr, &addrLen) == -1) {
        cerr << "Error getting client port: " << strerror(errno) << endl;
        return -1;
    }
    return ntohs(sAddr.sin_port);
    return 0;
}