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

const char* LOCALHOST_IP = "127.0.0.1";
const int LAST_3_DIGITS_YAXING_LI_USC_ID = 475;
const int SERVER_M_UDP_PORT = 44000 + LAST_3_DIGITS_YAXING_LI_USC_ID;
const int SERVER_M_TCP_PORT = 45000 + LAST_3_DIGITS_YAXING_LI_USC_ID;
const int BUFFER_SIZE = 1024;

int main() {
    // 1. Initial socket file descriptor for the client
    int cfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (cfd == -1) {
        std::cerr << "Initial Socket failed: " << std::strerror(errno) << std::endl;
        return 1;
    }

    // 2. Connect to the server port
    sockaddr_in sock_addr_in{};
    sock_addr_in.sin_family = AF_INET;
    sock_addr_in.sin_port = htons(SERVER_M_TCP_PORT);
    if (inet_pton(AF_INET, LOCALHOST_IP, &sock_addr_in.sin_addr) <= 0) {
        std::cerr << "Invalid IP address format." << std::endl;
        close(cfd);
        return 1;
    }

    if (connect(cfd, reinterpret_cast<struct sockaddr*>(&sock_addr_in), sizeof(sock_addr_in)) == -1) {
        std::cerr << "Connect failed: " << std::strerror(errno) << std::endl;
        close(cfd);
        return 1;
    }

    int number = 0;
    // 3. Communicate
    while (true) {
        std::string buff = "Hello World, " + std::to_string(number++) + "...\n";
        if (send(cfd, buff.c_str(), buff.size(), 0) == -1) {
            std::cerr << "Send failed: " << std::strerror(errno) << std::endl;
            close(cfd);
            return 1;
        }

        char buffer[BUFFER_SIZE];
        ssize_t len = recv(cfd, buffer, sizeof(buffer), 0);
        if (len > 0) {
            std::cout << "Data from Server: " << std::string(buffer, buffer + len) << std::endl;
        } else if (len == 0) {
            std::cout << "Server has disconnected" << std::endl;
            break;
        } else {
            std::cerr << "Receiving failed: " << std::strerror(errno) << std::endl;
            close(cfd);
            return 1;
        }

        sleep(1);
    }

    // Close the socket when done
    close(cfd);
    return 0;
}


/*
socket();
connect();
send();
recv();
close();
*/
