// lib.cpp

#include "libnew.h"
#include <string>

WebServer::WebServer() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Error creating socket" << std::endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    u_long on = 1;
    if (ioctlsocket(serverSocket, FIONBIO, &on) == SOCKET_ERROR) {
        std::cerr << "Error setting non-blocking mode" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    SOCKADDR_IN serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(12345);

    if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Error binding socket" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Error listening on socket" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    std::cout << "Server started on port 12345" << std::endl;
}

WebServer::~WebServer() {
    closesocket(serverSocket);
    WSACleanup();
}

void WebServer::handleRequests() {
    char buffer[1024];

    for (auto it = clients.begin(); it != clients.end(); ) {
        SOCKET clientSocket = it->first;
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            std::cout << "Received request from client " << clientSocket << ": " << buffer << std::endl;

            std::string request(buffer);

            size_t startPos = request.find("GET ") + 4;
            size_t endPos = request.find(" HTTP/");
            std::string path = request.substr(startPos, endPos - startPos);

            std::string response;
            if (routes.find(path) != routes.end()) {
                response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
                response += routes[path];
            } else {
                response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n";
                response += "<h1>404 Not Found</h1>";
            }

            send(clientSocket, response.c_str(), response.length(), 0);
            closesocket(clientSocket);
            it = clients.erase(it);
        } else if (bytesRead == 0) {
            std::cout << "Client disconnected: " << clientSocket << std::endl;
            closesocket(clientSocket);
            it = clients.erase(it);
        } else if (WSAGetLastError() != WSAEWOULDBLOCK) {
            std::cerr << "Error in recv from client " << clientSocket << std::endl;
            closesocket(clientSocket);
            it = clients.erase(it);
        } else {
            ++it;
        }
    }
}

void WebServer::acceptClient() {
    SOCKADDR_IN clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    SOCKET clientSocket = accept(serverSocket, (SOCKADDR*)&clientAddr, &clientAddrLen);
    if (clientSocket != INVALID_SOCKET) {
        std::cout << "Client connected" << std::endl;
        u_long on = 1;
        if (ioctlsocket(clientSocket, FIONBIO, &on) == SOCKET_ERROR) {
            std::cerr << "Error setting non-blocking mode for client socket" << std::endl;
            closesocket(clientSocket);
        } else {
            clients[clientSocket] = "";
        }
    }
}

void WebServer::start() {
    while (true) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(serverSocket, &readSet);

        timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int activity = select(0, &readSet, nullptr, nullptr, &timeout);
        if (activity == SOCKET_ERROR) {
            std::cerr << "Error in select" << std::endl;
            continue;
        }

        if (FD_ISSET(serverSocket, &readSet)) {
            acceptClient();
        }

        handleRequests();
    }
}

void WebServer::addRoute(const std::string& path, const std::string& response) {
    routes[path] = response;
}

int main() {
    WebServer server;

    std::string mainPageContent = R"(
        <!DOCTYPE html>
        <html lang="en">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>Main Page</title>
            <style>
                body {
                    font-family: Arial, sans-serif;
                    text-align: center;
                }
                h1 {
                    color: #333;
                }
                .container {
                    display: flex;
                    flex-direction: column;
                    align-items: center;
                    margin-top: 50px;
                }
                .button {
                    display: inline-block;
                    padding: 10px 20px;
                    margin: 10px;
                    background-color: #007bff;
                    color: #fff;
                    text-decoration: none;
                    border-radius: 5px;
                    transition: background-color 0.3s;
                }
                .button:hover {
                    background-color: #0056b3;
                }
            </style>
        </head>
        <body>
            <h1>Welcome to Dragon Ball Homepage</h1>
            <div class="container">
                <a class="button" href="/Goku">Visit Goku</a>
                <a class="button" href="/Vegeta">Visit Vegeta</a>
                <a class="button" href="/Gohan">Visit Gohan</a>
                <a class="button" href="/Piccolo">Visit Piccolo</a>
            </div>
        </body>
        </html>
    )";

    server.addRoute("/", mainPageContent);
    server.addRoute("/Goku", "<h1>Welcome to Goku's Profile</h1><p>Goku is the main protagonist of the Dragon Ball series.</p><a href=\"/\">Back to Home</a>");
    server.addRoute("/Vegeta", "<h1>Welcome to Vegeta's Profile</h1><p>Vegeta is a Saiyan prince and one of the most powerful characters in Dragon Ball.</p><a href=\"/\">Back to Home</a>");
    server.addRoute("/Gohan", "<h1>Welcome to Gohan's Profile</h1><p>Gohan is the eldest son of Goku and one of the main characters in Dragon Ball.</p><a href=\"/\">Back to Home</a>");
    server.addRoute("/Piccolo", "<h1>Welcome to Piccolo's Profile</h1><p>Piccolo is a Namekian warrior and one of Goku's allies.</p><a href=\"/\">Back to Home</a>");

    server.start();

    return 0;
}