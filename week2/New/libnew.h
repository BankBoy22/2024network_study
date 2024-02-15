#pragma once

#include <iostream>
#include <map>
#include <cstring>

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

class WebServer {
private:
    SOCKET serverSocket;
    std::map<SOCKET, std::string> clients;
    std::map<std::string, std::string> routes;

public:
    WebServer();

    ~WebServer();

    void handleRequests();

    void acceptClient();

    void start();

    void addRoute(const std::string& path, const std::string& response);
};