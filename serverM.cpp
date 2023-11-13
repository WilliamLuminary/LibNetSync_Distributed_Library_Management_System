//
// Created by Yaxing Li on 11/2/23.
//

#include <cstdio>
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

int initialize_udp_server(int port);

void receive_initial_book_statuses(int sfd, unordered_map<string, int> &bookStatuses);

void handle_commands(int sfd);

void send_acknowledgment(int sfd, const sockaddr_in &addr, const string &message);

unordered_map<string, int> deserialize_book_statuses(const string &data);

int initialize_tcp_server(int port);

int handle_new_connection(int sfd);

void communicate_with_client(int cfd);

int main() {
    int udp_sfd = initialize_udp_server(SERVER_M_UDP_PORT);
    if (udp_sfd < 0) {
        return 1;
    }

    unordered_map<string, int> combinedBookStatuses;
    receive_initial_book_statuses(udp_sfd, combinedBookStatuses);
    handle_commands(udp_sfd);
    close(udp_sfd); // Close the UDP socket

    int tcp_sfd = initialize_tcp_server(SERVER_M_TCP_PORT);
    if (tcp_sfd < 0) {
        return 1;
    }

    int client_fd = handle_new_connection(tcp_sfd);
    if (client_fd < 0) {
        close(tcp_sfd);
        return 1;
    }

    communicate_with_client(client_fd);

    close(client_fd);
    close(tcp_sfd); // Close the TCP socket
    return 0;
}


int initialize_udp_server(int port) {
    int sfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sfd == -1) {
        cerr << "Initial UDP Socket failed: " << strerror(errno) << endl;
        return -1;
    }

    struct sockaddr_in sock_addr_in{};
    sock_addr_in.sin_family = AF_INET;
    sock_addr_in.sin_port = htons(port);
    sock_addr_in.sin_addr.s_addr = INADDR_ANY;

    if (bind(sfd, (struct sockaddr *) &sock_addr_in, sizeof(sock_addr_in)) == -1) {
        cerr << "Bind UDP socket failed: " << strerror(errno) << endl;
        close(sfd);
        return -1;
    }

    return sfd;
}

void receive_initial_book_statuses(int sfd, unordered_map<string, int> &bookStatuses) {
    bool receivedS = false, receivedL = false, receivedH = false;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in sender_addr{};
    socklen_t sender_addr_len = sizeof(sender_addr);

    while (!(receivedS && receivedL && receivedH)) {
        ssize_t len = recvfrom(sfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &sender_addr, &sender_addr_len);
        if (len > 0) {
            string receivedData(buffer, len);
            auto tempStatuses = deserialize_book_statuses(receivedData);

            for (const auto &entry: tempStatuses) {
                bookStatuses[entry.first] = entry.second; // Insert into the main map
                if (entry.first[0] == 'S') receivedS = true;
                else if (entry.first[0] == 'L') receivedL = true;
                else if (entry.first[0] == 'H') receivedH = true;
            }

            send_acknowledgment(sfd, sender_addr, "Acknowledged");
        } else if (len < 0) {
            cerr << "Error receiving data: " << strerror(errno) << endl;
            break;
        }
    }
    cout << "Received all initial book statuses." << endl;
}

void handle_commands(int sfd) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in sender_addr{};
    socklen_t sender_addr_len = sizeof(sender_addr);

    while (true) {
        ssize_t len = recvfrom(sfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &sender_addr, &sender_addr_len);
        if (len > 0) {
            string command(buffer, len);
            cout << "Received command: " << command << endl;
            // TODO: Process the command
            send_acknowledgment(sfd, sender_addr, "Command processed"); // Placeholder response
        } else if (len < 0) {
            cerr << "Error receiving command: " << strerror(errno) << endl;
            break;
        }
    }
}

void send_acknowledgment(int sfd, const sockaddr_in &addr, const string &message) {
    ssize_t sent_bytes = sendto(sfd, message.c_str(), message.size(), 0, (const struct sockaddr *) &addr, sizeof(addr));
    if (sent_bytes < 0) {
        cerr << "Error sending acknowledgment: " << strerror(errno) << endl;
    }
}

unordered_map<string, int> deserialize_book_statuses(const string &data) {
    unordered_map<string, int> bookStatuses;
    std::stringstream ss(data);
    string line;

    while (std::getline(ss, line)) {
        std::stringstream lineStream(line);
        string bookCode;
        int status;
        if (std::getline(lineStream, bookCode, ',')) {
            lineStream >> status;
            bookStatuses[bookCode] = status;
        }
    }

    return bookStatuses;
}

int initialize_tcp_server(int port) {
    int sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sfd == -1) {
        cerr << "Initial TCP Socket failed: " << strerror(errno) << endl;
        return -1;
    }

    struct sockaddr_in sock_addr_in{};
    sock_addr_in.sin_family = AF_INET;
    sock_addr_in.sin_port = htons(port);
    sock_addr_in.sin_addr.s_addr = INADDR_ANY;

    int bind_result = bind(sfd, (struct sockaddr *) &sock_addr_in, sizeof(sock_addr_in));
    if (bind_result == -1) {
        cerr << "Bind TCP socket failed: " << strerror(errno) << endl;
        close(sfd);
        return -1;
    }

    int listen_result = listen(sfd, LISTEN_BACK_LOG);
    if (listen_result == -1) {
        cerr << "Listen TCP failed: " << strerror(errno) << endl;
        close(sfd);
        return -1;
    }

    return sfd;
}

int handle_new_connection(int sfd) {
    struct sockaddr_in caddr{};
    socklen_t caddr_len = sizeof(caddr);
    int cfd = accept(sfd, (struct sockaddr *) &caddr, &caddr_len);
    if (cfd == -1) {
        cerr << "Accept TCP failed: " << strerror(errno) << endl;
        return -1;
    }

    char ip[INET_ADDRSTRLEN];
    cout << "(TCP) IP Address of Client: " << inet_ntop(AF_INET, &caddr.sin_addr, ip, sizeof(ip))
         << ", Port Number: " << ntohs(caddr.sin_port) << endl;

    return cfd;
}

void communicate_with_client(int cfd) {
    char buff[BUFFER_SIZE];
    while (true) {
        ssize_t len = recv(cfd, buff, BUFFER_SIZE, 0);
        if (len > 0) {
            cout << "(TCP) Data from Client: " << string(buff, len) << endl;
            send(cfd, buff, len, 0); // Echo back the received data
        } else if (len == 0) {
            cout << "(TCP) Client has disconnected" << endl;
            break;
        } else {
            cerr << "(TCP) Receiving failed: " << strerror(errno) << endl;
            break;
        }
    }
}