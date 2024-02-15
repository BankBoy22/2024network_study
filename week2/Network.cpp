#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <cstring>

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

const int BUFFER_SIZE = 1024;

class WebServer {
private:
    SOCKET serverSocket;
    map<SOCKET, string> clients;
    map<string, string> routes; // routes 추가

public:
    WebServer() {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);

        serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (serverSocket == INVALID_SOCKET) {
            cerr << "Error creating socket" << endl;
            WSACleanup();
            exit(EXIT_FAILURE);
        }

        u_long on = 1;
        if (ioctlsocket(serverSocket, FIONBIO, &on) == SOCKET_ERROR) {
            cerr << "Error setting non-blocking mode" << endl;
            closesocket(serverSocket);
            WSACleanup();
            exit(EXIT_FAILURE);
        }
    }

    ~WebServer() {
        closesocket(serverSocket);
        WSACleanup();
    }

    void start(int port) {
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        serverAddr.sin_port = htons(port);

        if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            cerr << "Error binding socket" << endl;
            closesocket(serverSocket);
            WSACleanup();
            exit(EXIT_FAILURE);
        }

        if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
            cerr << "Error listening on socket" << endl;
            closesocket(serverSocket);
            WSACleanup();
            exit(EXIT_FAILURE);
        }

        cout << "Server started on port " << port << endl;

        while (true) {
            fd_set readSet;
            FD_ZERO(&readSet);
            FD_SET(serverSocket, &readSet);

            timeval timeout;
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;

            int activity = select(0, &readSet, nullptr, nullptr, &timeout);
            if (activity == SOCKET_ERROR) {
                cerr << "Error in select" << endl;
                continue;
            }

            if (FD_ISSET(serverSocket, &readSet)) {
                acceptClient();
            }

            handleRequests();
        }
    }

    void acceptClient() {
        sockaddr_in clientAddr;
        int clientAddrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket != INVALID_SOCKET) {
            cout << "Client connected" << endl;
            clients[clientSocket] = "";
        }
    }

    void handleRequests() {
        char buffer[BUFFER_SIZE];

        for (auto it = clients.begin(); it != clients.end(); ) {
            SOCKET clientSocket = it->first;
            int bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
            
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';
                cout << "Received request from client " << clientSocket << ": " << buffer << endl;
                
                string request(buffer);

                size_t startPos = request.find("GET ") + 4;
                size_t endPos = request.find(" HTTP/");
                string path = request.substr(startPos, endPos - startPos);

                if (routes.find(path) != routes.end()) {
                    string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
                    response += routes[path];
                    send(clientSocket, response.c_str(), response.length(), 0);
                } else {
                    const string notFoundResponse = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
                    send(clientSocket, notFoundResponse.c_str(), notFoundResponse.length(), 0);
                }
            } else if (bytesRead == 0) {
                cout << "Client disconnected: " << clientSocket << endl;
                closesocket(clientSocket);
                it = clients.erase(it);
            } else if (WSAGetLastError() != WSAEWOULDBLOCK) {
                cerr << "Error in recv from client " << clientSocket << endl;
                closesocket(clientSocket);
                it = clients.erase(it);
            } else {
                ++it;
            }
        }
    }

    void addRoute(const string& path, const string& response) {
        routes[path] = response;
    }
};

int main() {
    WebServer server;

    server.addRoute("/", "<h1>HI Nice To Meet you!!</h1>");
    server.addRoute("/Goku", "<h1>I'm Goku!!</h1>");
    server.addRoute("/Vegeta", "<h1>I'm Vegeta!!<h1>");
    server.addRoute("/Gohan", "<h1>I'm Gohan!!<h1>");
    server.addRoute("/Piccolo", "<h1>I'm Piccolo!!<h1>");

    server.start(12345);
}
