//
// Created by Yaxing Li on 11/13/23.
//

#ifndef SERVERM_H
#define SERVERM_H

#include <string>

const std::string FILE_PATH = "../";
const char *LOCALHOST_IP = "127.0.0.1";
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

std::unordered_map<std::string, std::string> read_member(const std::string &filepath);

int initialize_socket(int domain, int type, int protocol, int port, bool reuseaddr);

int accept_connection(int server_fd);

bool authenticate_client(int client_fd, const std::unordered_map<std::string, std::string> &memberInfo);

void handle_authenticated_tcp_requests(int client_fd, std::unordered_map<std::string, int> &bookStatuses, int udp_fd);

std::string determineServerIdentifier(const std::string &bookCode);

void forwardRequestToUdpServer(const std::string &serverIdentifier, const std::string &bookCode, int udpSocket);

std::string receiveResponseFromUdpServer(int udpSocket);

int getClientPort(int client_fd);

void send_acknowledgment(int sfd, const struct sockaddr_in &addr, const std::string &message);

void process_udp_server(int server_fd, std::unordered_map<std::string, int> &bookStatuses);

std::unordered_map<std::string, int> deserialize_book_statuses(const std::string &data);


#endif
