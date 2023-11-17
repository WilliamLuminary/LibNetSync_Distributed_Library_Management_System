//
// Created by Yaxing Li on 11/13/23.
//

// client.h
#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <utility>

const int BUFFER_SIZE = 1024;

class client {
public:
    void run();

private:
    const char *LOCALHOST_IP = "127.0.0.1";
    const int LAST_3_DIGITS_YAXING_LI_USC_ID = 475;
    const int SERVER_M_TCP_PORT = 45000 + LAST_3_DIGITS_YAXING_LI_USC_ID;
    int socket_fd = -1;
    std::string userName;

    bool setup();

    int createTcpSocket();

    bool connectToServer();

    bool authenticateAndHandleCommunication();

    void handleAuthenticatedTcpCommunication();

    bool sendBookCodeRequest(const std::string &bookCode);

    std::string receiveServerResponse();

    std::pair<std::string, std::string> promptForCredentials();

    void sendCredentials(const std::pair<std::string, std::string> &credentials);

    bool receiveAuthenticationResult(const std::string &username);

    void closeSocket();

    std::string encrypt(const std::string &input);

    int getHostPort(int socket_fd);

    void parseAdminResponse(const std::string &serverResponse, const std::string &bookCode);

    void parseNonAdminResponse(const std::string &serverResponse, const std::string &bookCode);

    void handleServerErrorResponse(const std::string &serverResponse, const std::string &bookCode);

};

#endif

