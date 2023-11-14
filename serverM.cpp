//
// Created by Yaxing Li on 11/2/23.
//

#include "serverM.h"

#include <cstdio>
#include <fstream>
#include <sstream>
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
using std::ifstream;
using std::getline;
using std::istringstream;


int main() {

    cout << "Main Server is up and running." << endl;

    int udp_fd = initialize_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, SERVER_M_UDP_PORT, true);
    if (udp_fd < 0) return 1;

    unordered_map<string, int> combinedBookStatuses;
    process_udp_server(udp_fd, combinedBookStatuses);


    auto memberInfo = read_member(FILE_PATH + "member.txt");

    int tcp_fd = initialize_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP, SERVER_M_TCP_PORT, true);
    if (tcp_fd < 0) return 1;
    while (true) {
        int client_fd = accept_connection(tcp_fd);
        if (client_fd < 0) continue;

        if (authenticate_client(client_fd, memberInfo)) {
            handle_authenticated_tcp_requests(client_fd, combinedBookStatuses, udp_fd);
        } else {
            cout << "Authentication failed. Closing connection." << endl;
            close(client_fd);
        }
    }

    close(tcp_fd);
    return 0;
}

int initialize_socket(int domain, int type, int protocol, int port, bool reuseaddr) {
    int fd = socket(domain, type, protocol);
    if (fd == -1) {
        cerr << "Socket creation failed: " << strerror(errno) << endl;
        return -1;
    }

    if (reuseaddr) {
        int opt_val = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));
    }

    sockaddr_in addr{};
    addr.sin_family = domain;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) == -1) {
        cerr << "Socket bind failed: " << strerror(errno) << endl;
        close(fd);
        return -1;
    }

    if (type == SOCK_STREAM) {
        if (listen(fd, LISTEN_BACK_LOG) == -1) {
            cerr << "Socket listen failed: " << strerror(errno) << endl;
            close(fd);
            return -1;
        }
    }

    return fd;
}

int accept_connection(int server_fd) {
    sockaddr_in client_addr{};
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd = accept(server_fd, reinterpret_cast<struct sockaddr *>(&client_addr), &client_addr_len);
    if (client_fd == -1) {
        cerr << "Accept failed: " << strerror(errno) << endl;
        return -1;
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
//    cout << "Connection accepted from " << client_ip << ":" << ntohs(client_addr.sin_port) << endl;

    return client_fd;
}

bool authenticate_client(int client_fd, const unordered_map<string, string> &memberInfo) {
    char buffer[BUFFER_SIZE];
    ssize_t len = recv(client_fd, buffer, BUFFER_SIZE, 0);

    if (len <= 0) {
        if (len < 0) cerr << "Error receiving credentials: " << strerror(errno) << endl;
        return false;
    }

    string credentials(buffer, len);
    size_t separator = credentials.find('\n');
    if (separator == string::npos) return false;

    string encryptedUsername = credentials.substr(0, separator);
    string encryptedPassword = credentials.substr(separator + 1, credentials.length() - separator - 1);

    cout << "Main Server received the username and password from the client using TCP over port " << SERVER_M_TCP_PORT
         << "." << endl;

    auto it = memberInfo.find(encryptedUsername);
    string response;

    if (it == memberInfo.end()) {
        response = "Failed login. Invalid username.";
        cout << encryptedUsername << " is not registered. Send a reply to the client." << endl;
    } else if (it->second != encryptedPassword) {
        response = "Failed login. Invalid password.";
        cout << "Password for " << encryptedUsername << " does not match the username. Send a reply to the client."
             << endl;
    } else {
        response = "Login successful.";
        cout << "Password for " << encryptedUsername << " matches the username. Send a reply to the client." << endl;
    }

    send(client_fd, response.c_str(), response.size(), 0);

    return response == "Login successful.";
}


void handle_authenticated_tcp_requests(int client_fd, unordered_map<string, int> &bookStatuses, int udp_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t len;

    while ((len = recv(client_fd, buffer, BUFFER_SIZE, 0)) > 0) {
        string bookCode(buffer, len);
        sockaddr_in client_addr{};
        socklen_t client_addr_len = sizeof(client_addr);
        getpeername(client_fd, (struct sockaddr *) &client_addr, &client_addr_len); // To get dynamic client port

        cout << "Main Server received the book request from client using TCP over port "
             << ntohs(client_addr.sin_port) << "." << endl;

        if (bookStatuses.find(bookCode) != bookStatuses.end()) {
            string serverIdentifier = determineServerIdentifier(bookCode);
            if (!serverIdentifier.empty()) {
                cout << "Found " << bookCode << " located at Server " << serverIdentifier
                     << ". Sending to Server " << serverIdentifier << "." << endl;
                forwardRequestToUdpServer(serverIdentifier, bookCode, udp_fd);
//                string udpResponse = receiveResponseFromUdpServer(udp_fd);
//                send(client_fd, udpResponse.c_str(), udpResponse.size(), 0);
                string udpResponse = receiveResponseFromUdpServer(udp_fd);
                cout << udpResponse << endl;
                send(client_fd, udpResponse.c_str(), udpResponse.size(), 0);
            } else {
                cerr << "Invalid book code prefix." << endl;
            }
        } else {
            string notFoundResponse = "Did not find " + bookCode + " in the book code list.";
            cout << notFoundResponse << endl;
            send(client_fd, notFoundResponse.c_str(), notFoundResponse.size(), 0);
        }
    }

    if (len == 0) {
        cout << "Client has disconnected." << endl;
    } else if (len < 0) {
        cerr << "Error receiving TCP request: " << strerror(errno) << endl;
    }
}


string determineServerIdentifier(const string &bookCode) {
    switch (bookCode[0]) {
        case 'S':
            return "S";
        case 'L':
            return "L";
        case 'H':
            return "H";
        default:
            return "";
    }
}

void forwardRequestToUdpServer(const string &serverIdentifier, const string &bookCode, int udpSocket) {
    struct sockaddr_in backendServerAddr{};
//    memset(&backendServerAddr, 0, sizeof(backendServerAddr));
    backendServerAddr.sin_family = AF_INET;
    backendServerAddr.sin_addr.s_addr = inet_addr(LOCALHOST_IP);

    if (serverIdentifier == "S") {
        backendServerAddr.sin_port = htons(SERVER_S_UDP_PORT);
    } else if (serverIdentifier == "L") {
        backendServerAddr.sin_port = htons(SERVER_L_UDP_PORT);
    } else if (serverIdentifier == "H") {
        backendServerAddr.sin_port = htons(SERVER_H_UDP_PORT);
    } else {
        cerr << "Invalid server identifier." << endl;
        return;
    }

    ssize_t sentBytes = sendto(udpSocket, bookCode.c_str(), bookCode.size(), 0,
                               (struct sockaddr *) &backendServerAddr, sizeof(backendServerAddr));
    if (sentBytes < 0) {
        cerr << "Error sending UDP request: " << strerror(errno) << endl;
    }
}

string receiveResponseFromUdpServer(int udpSocket) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in fromAddr{};
    socklen_t fromAddrLen = sizeof(fromAddr);

    ssize_t recvLen = recvfrom(udpSocket, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &fromAddr, &fromAddrLen);
    if (recvLen > 0) {
        return string(buffer, recvLen);
    } else {
        cerr << "Error receiving UDP response: " << strerror(errno) << endl;
        return "";
    }
}


int getClientPort(int client_fd) {
    sockaddr_in client_addr{};
    socklen_t addr_len = sizeof(client_addr);
    if (getpeername(client_fd, (struct sockaddr *) &client_addr, &addr_len) == -1) {
        cerr << "Error getting client port: " << strerror(errno) << endl;
        return -1;
    }
    return ntohs(client_addr.sin_port);
}


void process_udp_server(int server_fd, unordered_map<string, int> &bookStatuses) {
    bool receivedS = false, receivedL = false, receivedH = false;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in sender_addr{};
    socklen_t sender_addr_len = sizeof(sender_addr);

//    cout << "Waiting for initial book statuses from servers S, L, and H..." << endl;

    while (!receivedS || !receivedL || !receivedH) {
        ssize_t len = recvfrom(server_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &sender_addr,
                               &sender_addr_len);
        if (len > 0) {
            string receivedData(buffer, len);
            auto tempStatuses = deserialize_book_statuses(receivedData);
            string serverIdentifier;

            for (const auto &entry: tempStatuses) {
                bookStatuses[entry.first] = entry.second;

                if (!receivedS && entry.first[0] == 'S') {
                    receivedS = true;
                    serverIdentifier = "S";
                } else if (!receivedL && entry.first[0] == 'L') {
                    receivedL = true;
                    serverIdentifier = "L";
                } else if (!receivedH && entry.first[0] == 'H') {
                    receivedH = true;
                    serverIdentifier = "H";
                }
            }

            if (!serverIdentifier.empty()) {
                cout << "Main Server received the book code list from server " << serverIdentifier
                     << " using UDP over port " << ntohs(sender_addr.sin_port) << "." << endl;
//                send_acknowledgment(server_fd, sender_addr, "Acknowledged");
//                cout << "Sent acknowledgment to server " << serverIdentifier << "." << endl;
            }
        } else if (len < 0) {
            cerr << "Error receiving data: " << strerror(errno) << endl;
            if (errno == EINTR) continue; // Handle interrupted system calls
            else break;
        }
    }
//    if (receivedS && receivedL && receivedH) {
//        cout << "All initial book statuses have been received successfully." << endl;
//    }
}

void send_acknowledgment(int sfd, const sockaddr_in &addr, const string &message) {
    const char *msg = message.c_str();
    size_t msg_length = message.size();

    ssize_t bytes_sent = sendto(sfd, msg, msg_length, 0, (const struct sockaddr *) &addr, sizeof(addr));

    if (bytes_sent < 0) {
        cerr << "Error sending acknowledgment: " << strerror(errno) << endl;
    } else {
        cout << "Acknowledgment sent." << endl;
    }
}


unordered_map<string, int> deserialize_book_statuses(const string &data) {
    unordered_map<string, int> bookStatuses;
    istringstream ss(data);
    string line;

    while (getline(ss, line)) {
        istringstream lineStream(line);
        string bookCode;
        int status;
        if (getline(lineStream, bookCode, ',')) {
            lineStream >> status;  // Extract the status
            bookStatuses[bookCode] = status;
        }
    }
    return bookStatuses;
}

unordered_map<string, string> read_member(const string &filepath) {
    ifstream file(filepath);
    string line;
    unordered_map<string, string> memberInfo;

    if (file.is_open()) {
        while (getline(file, line)) {
            istringstream ss(line);
            string encryptedMemberName;
            string encryptedMemberPassword;
            if (getline(ss, encryptedMemberName, ',')) {
                ss.ignore();
                ss >> encryptedMemberPassword;
                memberInfo[encryptedMemberName] = encryptedMemberPassword;
            }
        }
        file.close();
    } else {
        cerr << "Unable to open file at path: " << filepath << endl;
    }

    cout << "Main Server loaded the member list." << endl;
    return memberInfo;
}