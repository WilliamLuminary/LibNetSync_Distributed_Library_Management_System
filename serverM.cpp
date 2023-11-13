//
// Created by Yaxing Li on 11/2/23.
//

#include <cstdio>
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

//const char* LOCALHOST_IP = "127.0.0.1";
//const int TCP_PROTOCOL = 0;
const int LAST_3_DIGITS_YAXING_LI_USC_ID = 475;
const int DEFAULT_UDP_PORT = 44000 + LAST_3_DIGITS_YAXING_LI_USC_ID;
const int DEFAULT_TCP_PORT = 45000 + LAST_3_DIGITS_YAXING_LI_USC_ID;
const int LISTEN_BACK_LOG = 128;
const int BUFFER_SIZE = 1024;

int tcp_kernel();

int udp_kernel();

int main() {
    udp_kernel();
    tcp_kernel();
    return 0;
}

int tcp_kernel() {
    // 1. Initial TCP socket file descriptor for the server
    int sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sfd == -1) {
        std::cerr << "Initial TCP Socket failed: " << strerror(errno) << std::endl;
        return 1;
    }

    // 2. Bind to the local port
    struct sockaddr_in sock_addr_in{};
    sock_addr_in.sin_family = AF_INET;
    sock_addr_in.sin_port = htons(DEFAULT_TCP_PORT);
    sock_addr_in.sin_addr.s_addr = INADDR_ANY;  //“INADDR_ANY” is 0, and no need to do any transformation to 0.
    // Besides, using this, the socket will set to the local IP.

    int bind_result = bind(sfd, reinterpret_cast<struct sockaddr *>(&sock_addr_in), sizeof(sock_addr_in));
    if (bind_result == -1) {
        std::cerr << "Bind TCP socket failed: " << strerror(errno) << std::endl;
        close(sfd);
        return 1;
    }

    // 3. Listen for the potential connection
    int listen_result = listen(sfd, LISTEN_BACK_LOG);
    if (listen_result == -1) {
        std::cerr << "Listen TCP failed: " << strerror(errno) << std::endl;
        close(sfd);
        return 1;
    }

    // 4. Block and wait for connection from clients
    struct sockaddr_in caddr{};
    socklen_t caddr_len = sizeof(caddr);
    int cfd = accept(sfd, reinterpret_cast<struct sockaddr *>(&caddr), &caddr_len);
    if (cfd == -1) {
        std::cerr << "Accept TCP failed: " << strerror(errno) << std::endl;
        close(sfd);
        return 1;
    }

    // Print the details on the connection
    char ip[INET_ADDRSTRLEN];
    std::cout << "(TCP), IP Address of Client: "
              << inet_ntop(AF_INET, &caddr.sin_addr, ip, sizeof(ip))
              << ", Port Number: " << ntohs(caddr.sin_port) << std::endl;

    // 5. Communicate
    char buff[BUFFER_SIZE];
    while (true) {
        ssize_t len = recv(cfd, buff, sizeof(buff), 0);
        if (len > 0) {
            std::cout << "(TCP) Data from Client: " << std::string(buff, len) << std::endl;
            send(cfd, buff, len, 0);
        } else if (len == 0) {
            std::cout << "(TCP) Client has disconnected" << std::endl;
            break;
        } else {
            std::cerr << "(TCP) Receiving failed: " << strerror(errno) << std::endl;
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
        std::cerr << "Initial UDP Socket failed: " << strerror(errno) << std::endl;
        return 1;
    }

    // 2. Bind to the local port
    struct sockaddr_in sock_addr_in{};
    sock_addr_in.sin_family = AF_INET;
    sock_addr_in.sin_port = htons(DEFAULT_UDP_PORT); // Replace with your actual port number
    sock_addr_in.sin_addr.s_addr = INADDR_ANY;

    int bind_result = bind(sfd, reinterpret_cast<struct sockaddr *>(&sock_addr_in), sizeof(sock_addr_in));
    if (bind_result == -1) {
        std::cerr << "Bind UDP socket failed: " << strerror(errno) << std::endl;
        close(sfd);
        return 1;
    }

    // 3. Communicate
    while (true) {
        // Buffer for incoming data
        char buffer[BUFFER_SIZE];
        struct sockaddr_in caddr{};
        socklen_t addr_len = sizeof(caddr);
        ssize_t len = recvfrom(sfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &caddr,
                               &addr_len);
        if (len >= 0) {
            if (len == 0) {
                std::cout << "(UDP) Backend Server has disconnected" << std::endl;
//            break;
            } else {
                std::cout << "(UDP) Data from Backend Server: " << std::string(buffer, buffer + len) << std::endl;
            }

            std::string response = "Acknowledged: " + std::string(buffer, buffer + len);
            sendto(sfd, response.c_str(), response.size(), 0, (struct sockaddr *) &caddr, addr_len);

        } else {
            std::cerr << "(UDP) Receiving failed: " << std::strerror(errno) << std::endl;
            if (errno == EINTR) {
                continue;
            }
            close(sfd);
            return 1;
        }
    }

    // 4. close all fds
    close(sfd);

    return 0;
}




