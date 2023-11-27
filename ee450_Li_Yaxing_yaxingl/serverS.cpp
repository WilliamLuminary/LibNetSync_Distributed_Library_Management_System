//
// Created by Yaxing Li on 11/2/23.
//

#include "serverS.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::unordered_map;

int main() {
    auto bookStatuses = read_book_list(FILE_PATH);

    int sock_fd = initialize_udp_socket(SERVER_S_UDP_PORT);
    if (sock_fd < 0) return 1;

    sockaddr_in serverMAddr = create_address(SERVER_M_UDP_PORT, LOCALHOST_IP);
    if (send_udp_data(sock_fd, serverMAddr, serialize_book_statuses(bookStatuses)) < 0) {
        close(sock_fd);
        return 1;
    }

    if (receive_udp_commands(sock_fd, bookStatuses) < 0) {
        close(sock_fd);
        return 1;
    }

    close(sock_fd);
    return 0;
}

int initialize_udp_socket(int port) {
    int sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_fd == -1) {
        cerr << "Could not create UDP socket: " << strerror(errno) << endl;
        return -1;
    }

    sockaddr_in addr = create_address(port);
    if (bind(sock_fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) == -1) {
        cerr << "Bind failed: " << strerror(errno) << endl;
        close(sock_fd);
        return -1;
    }

    cout << "Server " << SERVER_IDENTIFIER << " is up and running using UDP on port " << port << "." << endl;
    return sock_fd;
}

int send_udp_data(int sock_fd, const sockaddr_in &address, const string &data) {
    ssize_t sent_bytes = sendto(sock_fd, data.c_str(), data.size(), 0,
                                reinterpret_cast<const struct sockaddr *>(&address), sizeof(address));
    if (sent_bytes == -1) {
        cerr << "Sending UDP data failed: " << strerror(errno) << endl;
        return -1;
    }
    return 0;
}

sockaddr_in create_address(int port, const string &ip_address) {
//    sockaddr_in address{};
    sockaddr_in address;
    memset(&address, 0, sizeof(address));

    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (!ip_address.empty()) {
        inet_pton(AF_INET, ip_address.c_str(), &address.sin_addr);
    } else {
        address.sin_addr.s_addr = INADDR_ANY;
    }
    return address;
}

int receive_udp_commands(int sock_fd, unordered_map<string, int> &bookStatuses) {
    char buffer[BUFFER_SIZE];
//    sockaddr_in caddr{};
    sockaddr_in caddr;
    memset(&caddr, 0, sizeof(caddr));

    socklen_t caddr_len = sizeof(caddr);

    int portNum = getHostPort(sock_fd);
    while (true) {
        ssize_t len = recvfrom(sock_fd, buffer, BUFFER_SIZE, 0,
                               reinterpret_cast<struct sockaddr *>(&caddr), &caddr_len);
        if (len > 0) {
            buffer[len] = '\0';
            string receivedData(buffer, len);
            string response, bookCode;
            bool isInventoryType;
            if (receivedData.rfind(INVENTORY_QUERY_PREFIX, 0) == 0) {
                response = process_inventory_request(receivedData, bookStatuses, bookCode);
                isInventoryType = true;
            } else {
                response = process_book_request(receivedData, bookStatuses, bookCode);
                isInventoryType = false;
            }
            int sendResult = send_udp_data(sock_fd, caddr, response);
            if (sendResult < 0) {
                cerr << "Error sending UDP data: " << strerror(errno) << endl;
            } else {
                if (isInventoryType)
                    cout << "Server " << SERVER_IDENTIFIER
                         << " finished sending the inventory status to the Main server using UDP on port "
                         << portNum << "." << endl;
                else
                    cout << "Server " << SERVER_IDENTIFIER
                         << " finished sending the availability status of code " << bookCode
                         << " to the Main server using UDP on port "
                         << portNum << "." << endl;
            }
        } else if (len == -1 && errno != EINTR) {
            cerr << "Receiving UDP data failed: " << strerror(errno) << endl;
            return -1;
        }
    }
}

string process_inventory_request(const string &data, unordered_map<string, int> &bookStatuses, string &bookCode) {
    bookCode = extract_book_code(data);
    cout << "Server " << SERVER_IDENTIFIER << " received an inventory status request for code " << bookCode << "."
         << endl;
    auto it = bookStatuses.find(bookCode);
    if (it != bookStatuses.end()) {
        return INVENTORY_QUERY_PREFIX + bookCode + "," + std::to_string(it->second);
    } else {
        return "Not able to find the book";
    }
}

string process_book_request(const string &data, unordered_map<string, int> &bookStatuses, string &bookCode) {
    bookCode = extract_book_code(data);
    cout << "Server " << SERVER_IDENTIFIER << " received " << bookCode << " code from the Main Server." << endl;
    auto it = bookStatuses.find(bookCode);
    if (it == bookStatuses.end()) {
        return "Not able to find the book";
    }

    if (it->second > 0) {
        it->second -= 1;
        return "The requested book is available:" + bookCode + "," + std::to_string(it->second);
    } else {
        return "The requested book is not available";
    }
}

unordered_map<string, int> read_book_list(const string &filepath) {
    std::ifstream file(filepath);
    string line;
    unordered_map<string, int> bookStatuses;

    while (file && std::getline(file, line)) {
        std::istringstream ss(line);
        string bookCode;
        int status;
        if (std::getline(ss, bookCode, ',') && ss >> status) {
            bookStatuses[bookCode] = status;
        }
    }
    if (!file.eof()) {
        cerr << "Unable to read all data from file at path: " << filepath << endl;
    }
    return bookStatuses;
}

string serialize_book_statuses(const unordered_map<string, int> &bookStatuses) {
    std::ostringstream serializedData;
    for (const auto &entry: bookStatuses) {
        serializedData << entry.first << "," << entry.second << '\n';
    }
    return serializedData.str();
}

std::string extract_book_code(const std::string& data) {
    if (data.rfind(INVENTORY_QUERY_PREFIX, 0) == 0) {
        return data.substr(INVENTORY_QUERY_PREFIX.length());
    }
    size_t commaPos = data.find(',');
    return commaPos != std::string::npos ? data.substr(0, commaPos) : data;
}

int getHostPort(int socket_fd) {
//    sockaddr_in sAddr{};
    sockaddr_in sAddr;
    memset(&sAddr, 0, sizeof(sAddr));

    socklen_t addrLen = sizeof(sAddr);
    if (getsockname(socket_fd, (struct sockaddr *) &sAddr, &addrLen) == -1) {
        cerr << "Error getting client port: " << strerror(errno) << endl;
        return -1;
    }
    return ntohs(sAddr.sin_port);
}