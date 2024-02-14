#include <iostream>
#include <string>
#include <map>
#include <cstring>

#include <winsock2.h>
#include <ws2tcpip.h>

// 링크 라이브러리: ws2_32.lib
using namespace std;

// 버퍼 크기
const int BUFFER_SIZE = 1024;

// 웹 서버 클래스
class WebServer {
private:
    // 소켓 생성
    SOCKET serverSocket;
    // 라우팅 테이블
    map<string, string> routes;

public:
    // 생성자
    WebServer() {
        // WSADATA 구조체 초기화
        WSADATA wsaData;
        // 소켓 초기화
        WSAStartup(MAKEWORD(2, 2), &wsaData);

        // 소켓 생성
        serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (serverSocket == INVALID_SOCKET) 
        {
            cerr << "Error creating socket" << endl;
            WSACleanup();
            exit(EXIT_FAILURE);
        }

        // 논블로킹 소켓으로 설정
        u_long on = 1;
        if (ioctlsocket(serverSocket, FIONBIO, &on) == SOCKET_ERROR) 
        {
            cerr << "Error setting non-blocking mode" << endl;
            closesocket(serverSocket);
            WSACleanup();
            exit(EXIT_FAILURE);
        }
    }

    // 소멸자
    ~WebServer() {
        closesocket(serverSocket);
        WSACleanup();
    }

    // 서버 시작 start 메서드
    void start(int port) {
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        serverAddr.sin_port = htons(port);

        if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) 
        {
            cerr << "Error binding socket" << endl;
            closesocket(serverSocket);
            WSACleanup();
            exit(EXIT_FAILURE);
        }

        if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) 
        {
            cerr << "Error listening on socket" << endl;
            closesocket(serverSocket);
            WSACleanup();
            exit(EXIT_FAILURE);
        }

        // 서버 시작을 알림
        cout << "Server started on port " << port << endl;

        while (true) {
            // I/O 멀티플렉싱을 위한 fd_set 구조체 초기화
            fd_set readSet;
            FD_ZERO(&readSet);
            FD_SET(serverSocket, &readSet);

            // 타임아웃 설정
            timeval timeout;
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;

            // select 함수 호출
            int activity = select(0, &readSet, nullptr, nullptr, &timeout);
            // 에러 처리
            if (activity == SOCKET_ERROR) 
            {
                cerr << "Error in select" << endl;
                continue;
            }

            // 클라이언트가 접속했는지 확인
            if (FD_ISSET(serverSocket, &readSet)) 
            {
                sockaddr_in clientAddr;
                int clientAddrLen = sizeof(clientAddr);
                SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrLen);
                // 클라이언트 소켓이 유효하다면
                if (clientSocket != INVALID_SOCKET) 
                {
                    // 클라이언트 접속을 알림
                    cout << "Client connected" << endl;
                    // 요청 처리
                    handleRequest(clientSocket);
                    // 응답 전송

                    closesocket(clientSocket);
                    cout << "Client disconnected" << endl;
                }
            }
        }
    }

    // 라우팅 테이블 추가 이걸 통해서 HTML이나 다른 파일을 불러올 수 있음
    void addRoute(const string& path, const string& response) {
        routes[path] = response;
    }

// 요청 처리
private:
    void handleRequest(SOCKET clientSocket) 
    {
        while (true) {
            char buffer[BUFFER_SIZE];
            int bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';
                cout << "Received request: " << buffer << endl;

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
            } else if (bytesRead == -1) {
                // 클라이언트가 연결을 끊었을 때
                cout << "Client disconnected" << endl;
                closesocket(clientSocket);
                break;
            } else {
                // 오류 발생 시
                cerr << "Error in recv" << endl;
                closesocket(clientSocket);
                break;
            }
        }
    }
};

int main() {
    WebServer server;

    // 라우팅 테이블 설정
    server.addRoute("/", "<h1>HI Nice To Meet you!!</h1>");
    server.addRoute("/Goku", "<h1>I'm Goku!!</h1>");
    server.addRoute("/Vegeta", "<h1>I'm Vegeta!!</h1>");
    server.addRoute("/Picolo", "<h1>I'm Picolo!!</h1>");
    server.addRoute("/Gohan", "<h1>I'm Gohan!!</h1>");

    // 서버 시작
    server.start(12345);

    return 0;
}