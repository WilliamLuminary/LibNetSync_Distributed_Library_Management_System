//
// Created by Yaxing Li on 11/2/23.
//

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
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


const string FILE_PATH = "../";
const char *LOCALHOST_IP = "127.0.0.1";
const int LAST_3_DIGITS_YAXING_LI_USC_ID = 475;
const int SERVER_S_UDP_BASE_PORT = 41000;
const int SERVER_S_UDP_PORT = SERVER_S_UDP_BASE_PORT + LAST_3_DIGITS_YAXING_LI_USC_ID;
const int SERVER_M_UDP_BASE_PORT = 44000;
const int SERVER_M_UDP_PORT = SERVER_M_UDP_BASE_PORT + LAST_3_DIGITS_YAXING_LI_USC_ID;
const int BUFFER_SIZE = 1024;


int initialize_udp_socket(int port);

int send_udp_data(int sockfd, const sockaddr_in &address, const string &data);

int receive_udp_commands(int sockfd);

unordered_map<string, int> read_book_list(const string &filepath);

void print_map(const unordered_map<string, int> &bookStatuses);

string serialize_book_statuses(const unordered_map<string, int> &bookStatuses);

sockaddr_in create_address(int port, const string &ip_address);

int main() {
    auto bookStatuses = read_book_list(FILE_PATH + "science.txt");
//    print_map(bookStatuses);

    int sockfd = initialize_udp_socket(0); // Port 0 lets the OS choose the port
    if (sockfd < 0) return 1;

    sockaddr_in serverMAddr = create_address(SERVER_M_UDP_PORT, LOCALHOST_IP);
    if (send_udp_data(sockfd, serverMAddr, serialize_book_statuses(bookStatuses)) < 0) {
        close(sockfd);
        return 1;
    }

    // Now, Server S switches to server mode to receive commands from ServerM
    if (receive_udp_commands(sockfd) < 0) {
        close(sockfd);
        return 1;
    }

    close(sockfd);
    return 0;
}

int initialize_udp_socket(int port) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == -1) {
        cerr << "Could not create UDP socket: " << strerror(errno) << endl;
        return -1;
    }
    sockaddr_in addr = create_address(port, "");
    if (bind(sockfd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) == -1) {
        cerr << "Bind failed: " << strerror(errno) << endl;
        close(sockfd);
        return -1;
    }
    return sockfd;
}

int send_udp_data(int sockfd, const sockaddr_in &address, const string &data) {
    ssize_t sent_bytes = sendto(sockfd, data.c_str(), data.size(), 0,
                                reinterpret_cast<const struct sockaddr *>(&address), sizeof(address));
    if (sent_bytes == -1) {
        cerr << "Sending UDP data failed: " << strerror(errno) << endl;
        return -1;
    }
    cout << "Sent UDP data to server." << endl;
    return 0;
}


sockaddr_in create_address(int port, const string &ip_address) {
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (!ip_address.empty()) {
        inet_pton(AF_INET, ip_address.c_str(), &address.sin_addr);
    } else {
        address.sin_addr.s_addr = INADDR_ANY;
    }
    return address;
}


unordered_map<string, int> read_book_list() {

    std::ifstream file(FILE_PATH + "science.txt");
    string line;
    unordered_map<string, int> bookStatuses;

    if (file.is_open()) {
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            string bookCode;
            int status;
            if (getline(ss, bookCode, ',')) {
                ss.ignore();
                ss >> status;
                bookStatuses[bookCode] = status;
            }
        }
        file.close();
    } else {
        cerr << "Unable to open file" << endl;
    }
    return bookStatuses;
}

int receive_udp_commands(int sockfd) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in caddr{};
    socklen_t caddr_len = sizeof(caddr);

    while (true) {
        ssize_t len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                               reinterpret_cast<struct sockaddr *>(&caddr), &caddr_len);
        if (len > 0) {
            string command(buffer, len);
            cout << "Received command: " << command << endl;

            // TODO: Process the command here

            // Sending a response can be implemented here if necessary
        } else if (len == -1) {
            cerr << "Receiving UDP data failed: " << strerror(errno) << endl;
            if (errno == EINTR) {
                continue; // Handle interrupted system calls
            }
            return -1; // An error occurred
        }
        // Since UDP is connectionless, a len of 0 does not indicate 'disconnection'
        // as it would in a stream-oriented protocol like TCP
    }

    return 0; // Never reached unless you add a condition to break the loop
}

unordered_map<string, int> read_book_list(const string &filepath) {
    std::ifstream file(filepath);
    string line;
    unordered_map<string, int> bookStatuses;

    if (file.is_open()) {
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            string bookCode;
            int status;
            if (std::getline(ss, bookCode, ',')) {
                ss.ignore();
                ss >> status;
                bookStatuses[bookCode] = status;
            }
        }
        file.close();
    } else {
        cerr << "Unable to open file at path: " << filepath << endl;
        // Handle error as necessary
    }
    return bookStatuses;
}

void print_map(const unordered_map<string, int> &bookStatuses) {
    for (const auto &pair: bookStatuses) {
        cout << "Book Code: " << pair.first << ", Status: " << pair.second << endl;
    }
}

string serialize_book_statuses(const unordered_map<string, int> &bookStatuses) {
    std::stringstream serializedData;
    for (const auto &entry: bookStatuses) {
        serializedData << entry.first << "," << entry.second << "\n";
    }
    return serializedData.str();
}