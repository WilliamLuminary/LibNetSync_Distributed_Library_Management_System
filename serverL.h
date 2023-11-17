//
// Created by Yaxing Li on 11/13/23.
//

#ifndef SERVER_L_H
#define SERVER_L_H

#include <string>


const std::string FILE_PATH = "../literature.txt";
const char *LOCALHOST_IP = "127.0.0.1";
const int LAST_3_DIGITS_YAXING_LI_USC_ID = 475;
const int SERVER_L_UDP_BASE_PORT = 42000;
const int SERVER_L_UDP_PORT = SERVER_L_UDP_BASE_PORT + LAST_3_DIGITS_YAXING_LI_USC_ID;
const int SERVER_M_UDP_BASE_PORT = 44000;
const int SERVER_M_UDP_PORT = SERVER_M_UDP_BASE_PORT + LAST_3_DIGITS_YAXING_LI_USC_ID;
const int BUFFER_SIZE = 1024;
const std::string SERVER_IDENTIFIER = "L";
const std::string INVENTORY_QUERY_PREFIX = "INVENTORY:";


int initialize_udp_socket(int port);

int send_udp_data(int sockfd, const struct sockaddr_in &address, const std::string &data);

int receive_udp_commands(int sockfd, std::unordered_map<std::string, int> &bookStatuses);

std::string process_book_request(const std::string &bookCode, std::unordered_map<std::string, int> &bookStatuses);

std::unordered_map<std::string, int> read_book_list(const std::string &filepath);

void update_book_list_file(const std::unordered_map<std::string, int> &bookStatuses);

void print_map(const std::unordered_map<std::string, int> &bookStatuses);

std::string serialize_book_statuses(const std::unordered_map<std::string, int> &bookStatuses);

sockaddr_in create_address(int port, const std::string &ip_address);

std::string extract_book_code(const std::string &data);

std::string process_inventory_request(const std::string &bookCode, std::unordered_map<std::string, int> &bookStatuses);

#endif
