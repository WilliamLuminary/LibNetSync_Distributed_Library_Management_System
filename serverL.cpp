//
// Created by Yaxing Li on 11/2/23.
//

#include "serverL.h"

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


int main() {
    auto bookStatuses = read_book_list(FILE_PATH);
//    print_map(bookStatuses);

    int sockfd = initialize_udp_socket(SERVER_L_UDP_PORT); // Port 0 lets the OS choose the port
    if (sockfd < 0) return 1;

    sockaddr_in serverMAddr = create_address(SERVER_M_UDP_PORT, LOCALHOST_IP);
    if (send_udp_data(sockfd, serverMAddr, serialize_book_statuses(bookStatuses)) < 0) {
        close(sockfd);
        return 1;
    }

    if (receive_udp_commands(sockfd, bookStatuses) < 0) {
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

    cout << "Server " << SERVER_IDENTIFIER << " is up and running using UDP on port " << port << "." << endl;
    return sockfd;
}


int send_udp_data(int sockfd, const sockaddr_in &address, const string &data) {
    ssize_t sent_bytes = sendto(sockfd, data.c_str(), data.size(), 0,
                                reinterpret_cast<const struct sockaddr *>(&address), sizeof(address));
    if (sent_bytes == -1) {
        cerr << "Sending UDP data failed: " << strerror(errno) << endl;
        return -1;
    }
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

int receive_udp_commands(int sockfd, unordered_map<string, int> &bookStatuses) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in caddr{};
    socklen_t caddr_len = sizeof(caddr);

    while (true) {
        ssize_t len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                               reinterpret_cast<struct sockaddr *>(&caddr), &caddr_len);
        if (len > 0) {
            buffer[len] = '\0';
            string receivedData(buffer, len);

            if (receivedData.rfind("INVENTORY:", 0) == 0) {
                string bookCode = extract_book_code(receivedData);
                string response = process_inventory_request(bookCode, bookStatuses);

                send_udp_data(sockfd, caddr, response);

                cout << "Server " << SERVER_IDENTIFIER
                     << " finished sending the inventory status to the Main server using UDP on port "
                     << ntohs(caddr.sin_port) << "." << endl;
            } else {
                string bookCode = extract_book_code(receivedData);
                string response = process_book_request(bookCode, bookStatuses);

                send_udp_data(sockfd, caddr, response);
            }
        } else if (len == -1) {
            cerr << "Receiving UDP data failed: " << strerror(errno) << endl;
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
    }

    return 0;
}

string process_book_request(const string &bookCode, unordered_map<string, int> &bookStatuses) {
    auto it = bookStatuses.find(bookCode);
    if (it != bookStatuses.end()) {
        if (it->second > 0) {
            it->second -= 1;
            // Update the book list file with new count here, if necessary
            update_book_list_file(bookStatuses);

            return "The requested book is available";
        } else {
            return "The requested book is not available";
        }
    } else {
        return "Not able to find the book";
    }
}

string process_inventory_request(const string &bookCode, unordered_map<string, int> &bookStatuses) {
    auto it = bookStatuses.find(bookCode);
    string response;
    if (it != bookStatuses.end()) {
        response = INVENTORY_QUERY_PREFIX + bookCode + "," + std::to_string(it->second);
    } else {
        response = INVENTORY_QUERY_PREFIX + bookCode + ",0";
    }

    cout << "Server " << SERVER_IDENTIFIER << " received an inventory status request for code " << bookCode << "."
         << endl;
    return response;
}


void update_book_list_file(const unordered_map<string, int> &bookStatuses) {
    std::ofstream file(FILE_PATH, std::ios::trunc);
    if (!file.is_open() || file.fail()) {
        cerr << "Unable to open file at path: " << FILE_PATH << " to update book list." << endl;
        return;
    }

    for (const auto &pair: bookStatuses) {
        file << pair.first << "," << pair.second << std::endl;

        if (file.fail()) {
            cerr << "Failed to write to file at path: " << FILE_PATH << endl;
            break;
        }
    }

    file.close();
    if (file.fail()) {
        cerr << "Failed to properly close file at path: " << FILE_PATH << endl;
    }
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
                ss >> status; // Directly read the integer status without ignoring any characters.
                bookStatuses[bookCode] = status;
            }
        }
        file.close();
    } else {
        cerr << "Unable to open file at path: " << filepath << endl;
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

string extract_book_code(const string &data) {
    if (data.rfind(INVENTORY_QUERY_PREFIX, 0) == 0) {
        return data.substr(INVENTORY_QUERY_PREFIX.length());
    } else {
        std::istringstream iss(data);
        string bookCode;
        getline(iss, bookCode, ',');
        return bookCode;
    }
}