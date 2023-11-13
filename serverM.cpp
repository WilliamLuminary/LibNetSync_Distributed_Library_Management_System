//
// Created by Yaxing Li on 11/2/23.
//

#include <cstdio>
#include <sstream>  // Include this at the top of your file

#include <cstdlib>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <iostream>


using std::cout;
using std::cerr;
using std::endl;
using std::cin;
using std::unordered_map;
using std::string;
using std::strerror;


const int LAST_3_DIGITS_YAXING_LI_USC_ID = 475;
const int SERVER_S_UDP_BASE_PORT = 41000;
const int SERVER_S_UDP_PORT = SERVER_S_UDP_BASE_PORT + LAST_3_DIGITS_YAXING_LI_USC_ID;
const int SERVER_L_UDP_BASE_PORT = 42000;
const int SERVER_L_UDP_PORT = SERVER_L_UDP_BASE_PORT + LAST_3_DIGITS_YAXING_LI_USC_ID;
const int SERVER_H_UDP_BASE_PORT = 43000;
const int SERVER_H_UDP_PORT = SERVER_H_UDP_BASE_PORT + LAST_3_DIGITS_YAXING_LI_USC_ID;
const int SERVER_M_UDP_BASE_PORT = 44000;
const int SERVER_M_UDP_PORT = SERVER_M_UDP_BASE_PORT + LAST_3_DIGITS_YAXING_LI_USC_ID;
const int SERVER_M_TCP_BASE_PORT = 45000;
const int SERVER_M_TCP_PORT = SERVER_M_TCP_BASE_PORT + LAST_3_DIGITS_YAXING_LI_USC_ID;
const int LISTEN_BACK_LOG = 128;
const int BUFFER_SIZE = 1024;

bool starts_with(const std::string &fullString, const std::string &prefix);

int tcp_kernel();

int udp_kernel();

unordered_map<string, int> deserialize_book_statuses(const string &data);

int main() {
    udp_kernel();
    tcp_kernel();
    return 0;
}

int tcp_kernel() {
    // 1. Initial TCP socket file descriptor for the server
    int sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sfd == -1) {
        cerr << "Initial TCP Socket failed: " << strerror(errno) << endl;
        return 1;
    }

    // 2. Bind to the local port
    struct sockaddr_in sock_addr_in{};
    sock_addr_in.sin_family = AF_INET;
    sock_addr_in.sin_port = htons(SERVER_M_TCP_PORT);
    sock_addr_in.sin_addr.s_addr = INADDR_ANY;  //“INADDR_ANY” is 0, and no need to do any transformation to 0.
    // Besides, using this, the socket will set to the local IP.

    int bind_result = bind(sfd, reinterpret_cast<struct sockaddr *>(&sock_addr_in), sizeof(sock_addr_in));
    if (bind_result == -1) {
        cerr << "Bind TCP socket failed: " << strerror(errno) << endl;
        close(sfd);
        return 1;
    }

    // 3. Listen for the potential connection
    int listen_result = listen(sfd, LISTEN_BACK_LOG);
    if (listen_result == -1) {
        cerr << "Listen TCP failed: " << strerror(errno) << endl;
        close(sfd);
        return 1;
    }

    // 4. Block and wait for connection from clients
    struct sockaddr_in caddr{};
    socklen_t caddr_len = sizeof(caddr);
    int cfd = accept(sfd, reinterpret_cast<struct sockaddr *>(&caddr), &caddr_len);
    if (cfd == -1) {
        cerr << "Accept TCP failed: " << strerror(errno) << endl;
        close(sfd);
        return 1;
    }

    // Print the details on the connection
    char ip[INET_ADDRSTRLEN];
    cout << "(TCP), IP Address of Client: "
         << inet_ntop(AF_INET, &caddr.sin_addr, ip, sizeof(ip))
         << ", Port Number: " << ntohs(caddr.sin_port) << endl;

    // 5. Communicate
    char buff[BUFFER_SIZE];
    while (true) {
        ssize_t len = recv(cfd, buff, sizeof(buff), 0);
        if (len > 0) {
            cout << "(TCP) Data from Client: " << string(buff, len) << endl;
            send(cfd, buff, len, 0);
        } else if (len == 0) {
            cout << "(TCP) Client has disconnected" << endl;
            break;
        } else {
            cerr << "(TCP) Receiving failed: " << strerror(errno) << endl;
            break;
        }
    }

    // 6. close all fds
    close(sfd);
    close(cfd);
    return 0;
}

int udp_kernel() {
    // 1. Initial UDP socket file descriptor for the server
    int sfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sfd == -1) {
        cerr << "Initial UDP Socket failed: " << strerror(errno) << endl;
        return 1;
    }

    // 2. Bind to the local port
    struct sockaddr_in sock_addr_in{};
    sock_addr_in.sin_family = AF_INET;
    sock_addr_in.sin_port = htons(SERVER_M_UDP_PORT); // Replace with your actual port number
    sock_addr_in.sin_addr.s_addr = INADDR_ANY;

    int bind_result = bind(sfd, reinterpret_cast<struct sockaddr *>(&sock_addr_in), sizeof(sock_addr_in));
    if (bind_result == -1) {
        cerr << "Bind UDP socket failed: " << strerror(errno) << endl;
        close(sfd);
        return 1;
    }

    unordered_map<string, int> combinedBookStatuses;

    // 3. Communicate
    bool receivedS = false, receivedL = false, receivedH = false;

    while (!(receivedS && receivedL && receivedH)) {
        struct sockaddr_in caddr{};
        socklen_t addr_len = sizeof(caddr);
        char buffer[BUFFER_SIZE];

        ssize_t len = recvfrom(sfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&caddr, &addr_len);
        if (len > 0) {
            std::string receivedData(buffer, len);
            auto bookStatuses = deserialize_book_statuses(receivedData);

            // Process each received book status
            for (const auto& entry : bookStatuses) {
                char serverIdentifier = entry.first[0]; // Assume the first character is the server identifier
                combinedBookStatuses[entry.first] = entry.second;

                // Check the identifier to set the flags
                if (serverIdentifier == 'S') receivedS = true;
                else if (serverIdentifier == 'L') receivedL = true;
                else if (serverIdentifier == 'H') receivedH = true;
            }

            // Send an acknowledgment back to the backend server
            std::string response = "Acknowledged";
            sendto(sfd, response.c_str(), response.size(), 0, (struct sockaddr *)&caddr, addr_len);
        } else if (len == -1) {
            std::cerr << "(UDP) Receiving failed: " << std::strerror(errno) << std::endl;
            if (errno == EINTR) continue; // Handle interrupted system calls
            break; // Exit on other errors
        }
    }

    // At this point, all book statuses have been received
    std::cout << "Received all book statuses from servers S, L, and H." << std::endl;



    // Main Communication Loop: Handle ongoing commands
    struct sockaddr_in caddr{};
    socklen_t addr_len = sizeof(caddr);
    char buffer[BUFFER_SIZE];
    while (true) {
        addr_len = recvfrom(sfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &caddr, &addr_len);
        if (addr_len > 0) {
            std::string command(buffer, addr_len);
            // Process the command here...
            std::cout << "(UDP) Command received: " << command << std::endl;

            // Send response for the command
            std::string commandResponse = "Command Response";
            sendto(sfd, commandResponse.c_str(), commandResponse.size(), 0, (struct sockaddr *) &caddr, addr_len);
        } else if (addr_len == -1) {
            std::cerr << "(UDP) Command receiving failed: " << std::strerror(errno) << std::endl;
            if (errno == EINTR) continue; // Handle interrupted system calls
            // Break the loop if there's an error other than an interruption
            break;
        } else if (addr_len == 0) {
            // This block is likely unnecessary for UDP since a zero-length datagram is valid.
            std::cout << "(UDP) Server has disconnected" << std::endl;
            break;
        }
    }

    // 4. close all fds
    close(sfd);

    return 0;
}


unordered_map<string, int> deserialize_book_statuses(const string &data) {
    unordered_map<string, int> bookStatuses;
    std::stringstream ss(data);
    string item;

    while (std::getline(ss, item, '\n')) {
        std::stringstream itemStream(item);
        string bookCode;
        int status;

        if (std::getline(itemStream, bookCode, ',')) {
            itemStream >> status;
            bookStatuses[bookCode] = status;
        }
    }

    return bookStatuses;
}

bool starts_with(const std::string &fullString, const std::string &prefix) {
    if (fullString.length() < prefix.length()) {
        return false;
    }
    return fullString.compare(0, prefix.length(), prefix) == 0;
}


