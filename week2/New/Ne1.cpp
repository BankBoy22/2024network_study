#include <iostream>
#include <WinSock2.h>
#include <Windows.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib") 
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <memory>

using namespace std;

class MemoryPool {
    size_t blockSize;
    vector<char*> freeBlocks;
    mutex mtx;

public:
    explicit MemoryPool(size_t blockSize, size_t reserve = 0) : blockSize(blockSize) {
        freeBlocks.reserve(reserve);
        for (size_t i = 0; i < reserve; i++) {
            freeBlocks.push_back(new char[blockSize]);
        }
    }

    ~MemoryPool() {
        for (char* block : freeBlocks) {
            delete[] block;
        }
    }

    void* alloc() {
        lock_guard<mutex> lock(mtx);
        void* block;

        if (!freeBlocks.empty()) {
            block = freeBlocks.back();
            freeBlocks.pop_back();
        }
        else {
            block = new char[blockSize];
        }

        return block;
    }

    void dealloc(void* ptr) {
        lock_guard<mutex> lock(mtx);
        if (ptr != nullptr) {
            freeBlocks.push_back(reinterpret_cast<char*>(ptr));
        }
    }

    void resize(size_t addreserve) {
        lock_guard<mutex> lock(mtx);
        freeBlocks.reserve(freeBlocks.capacity() + addreserve);
        for (size_t i = 0; i < addreserve; i++) {
            freeBlocks.push_back(new char[blockSize]);
        }
    }
};

class WebServer {
    SOCKET serverSocket;
    MemoryPool& memoryPool;
    unordered_map<SOCKET, string> clients;
    atomic<bool> isRunning;

public:
    WebServer(MemoryPool& pool) : memoryPool(pool), isRunning(true) {}

    ~WebServer() {
        closesocket(serverSocket);
        WSACleanup();
    }

    void start(int port) {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);

        serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (serverSocket == INVALID_SOCKET) {
            cerr << "Error creating socket" << endl;
            WSACleanup();
            return;
        }

        SOCKADDR_IN serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        serverAddr.sin_port = htons(port);

        if (bind(serverSocket, reinterpret_cast<SOCKADDR*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
            cerr << "Error binding socket" << endl;
            closesocket(serverSocket);
            WSACleanup();
            return;
        }

        if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
            cerr << "Error listening on socket" << endl;
            closesocket(serverSocket);
            WSACleanup();
            return;
        }

        cout << "Server started on port " << port << endl;

        while (isRunning) {
            acceptClient();
            handleRequests();
        }
    }

    void stop() {
        isRunning = false;
    }

private:
    void acceptClient() {
        SOCKADDR_IN clientAddr;
        int clientAddrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, reinterpret_cast<SOCKADDR*>(&clientAddr), &clientAddrLen);
        if (clientSocket != INVALID_SOCKET) {
            cout << "Client connected" << endl;
            u_long on = 1;
            if (ioctlsocket(clientSocket, FIONBIO, &on) == SOCKET_ERROR) {
                cerr << "Error setting non-blocking mode for client socket" << endl;
                closesocket(clientSocket);
            } else {
                clients[clientSocket] = "";
            }
        }
    }

    void handleRequests() {
        char* buffer = static_cast<char*>(memoryPool.alloc());

        for (auto it = clients.begin(); it != clients.end(); ) {
            SOCKET clientSocket = it->first;
            int bytesRead = recv(clientSocket, buffer, 1024, 0);

            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';
                cout << "Received request from client " << clientSocket << ": " << buffer << endl;

                string request(buffer);

                size_t startPos = request.find("GET ") + 4;
                size_t endPos = request.find(" HTTP/");
                string path = request.substr(startPos, endPos - startPos);

                string response;
                if (path == "/") {
                    response = "HTTP/1.1 200 OK\r\n";
                    response += "Content-Type: text/html\r\n\r\n";
                    response += "<h1>Welcome to Dragon Ball Homepage</h1>";
                    response += "<a href=\"/Goku\">Visit Goku</a><br>";
                    response += "<a href=\"/Vegeta\">Visit Vegeta</a><br>";
                    response += "<a href=\"/Gohan\">Visit Gohan</a><br>";
                    response += "<a href=\"/Piccolo\">Visit Piccolo</a><br>";
                } else if (path == "/Goku") {
                    response = "HTTP/1.1 200 OK\r\n";
                    response += "Content-Type: text/html\r\n\r\n";
                    response += "<h1>Welcome to Goku's Profile</h1><p>Goku is the main protagonist of the Dragon Ball series.</p><a href=\"/\">Back to Home</a>";
                } else if (path == "/Vegeta") {
                    response = "HTTP/1.1 200 OK\r\n";
                    response += "Content-Type: text/html\r\n\r\n";
                    response += "<h1>Welcome to Vegeta's Profile</h1><p>Vegeta is a Saiyan prince and one of the most powerful characters in Dragon Ball.</p><a href=\"/\">Back to Home</a>";
                } else if (path == "/Gohan") {
                    response = "HTTP/1.1 200 OK\r\n";
                    response += "Content-Type: text/html\r\n\r\n";
                    response += "<h1>Welcome to Gohan's Profile</h1><p>Gohan is the eldest son of Goku and one of the main characters in Dragon Ball.</p><a href=\"/\">Back to Home</a>";
                } else if (path == "/Piccolo") {
                    response = "HTTP/1.1 200 OK\r\n";
                    response += "Content-Type: text/html\r\n\r\n";
                    response += "<h1>Welcome to Piccolo's Profile</h1><p>Piccolo is a Namekian warrior and one of Goku's allies.</p><a href=\"/\">Back to Home</a>";
                } else {
                    response = "HTTP/1.1 404 Not Found\r\n";
                    response += "Content-Type: text/html\r\n\r\n";
                    response += "<h1>404 Not Found</h1>";
                }

                send(clientSocket, response.c_str(), response.length(), 0);
                closesocket(clientSocket);
                it = clients.erase(it);
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

        memoryPool.dealloc(buffer);
    }
};

int main() {
    MemoryPool pool(sizeof(char) * 1024, 100); // 블록 크기와 초기 예약 크기 설정
    WebServer server(pool);
    server.start(12345);
    return 0;
}
