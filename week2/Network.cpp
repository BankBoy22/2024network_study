#include "lib.h"
#include <fstream>
#include <sstream>

using namespace std;

// 파일을 읽어서 문자열로 반환하는 함수
string readFileToString(const string& filename) {
    ifstream file(filename); // 파일 열기
    stringstream ss; // 문자열 스트림 생성
    if (!file.is_open()) { // 파일이 열리지 않은 경우
        cerr << "Unable to open file: " << filename << endl;
        return ""; // 빈 문자열 반환
    }
    ss << file.rdbuf(); // 파일 내용을 스트림에 쓰기
    return ss.str(); // 스트림을 문자열로 반환
}

int main() {
    // 네트워크 라이브러리 초기화
    WSAData wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // Non-blocking Socket
    SOCKET servsock = socket(AF_INET, SOCK_STREAM, 0); // TCP 소켓 생성
    if (servsock == INVALID_SOCKET) { // 소켓 생성 실패시
        cout << "socket() error" << endl; // 에러 메시지 출력
        return 0;
    }

    // 논블로킹 소켓으로 만들기
    u_long on = 1; // 1로 설정하면 논블로킹 소켓으로 만들어준다
    if (ioctlsocket(servsock, FIONBIO, &on) == SOCKET_ERROR) { // 소켓을 논블로킹으로 만들기
        cout << "ioctlsocket() error" << endl; // 에러 메시지 출력
        return 0;
    }

    // 서버 주소 설정
    SOCKADDR_IN servaddr;
    memset(&servaddr, 0, sizeof(servaddr)); // 0으로 초기화
    servaddr.sin_family = AF_INET; // IPv4 주소체계 사용
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // 서버의 IP 주소 설정
    servaddr.sin_port = htons(12345); // 서버의 포트 번호 설정 (12345번 포트 사용)

    // 논블로킹 소켓은 bind()에서도 루프를 돌면서 될 때까지 계속 시도해야함 
    // 클라이언트가 접속을 끊었을 때, 서버가 바인딩을 해제하고 다시 바인딩을 시도해야함
    while (bind(servsock, (SOCKADDR*)&servaddr, sizeof(servaddr)) == SOCKET_ERROR) {
        if (WSAGetLastError() == WSAEWOULDBLOCK) {
            // 논블로킹 소켓에서는 루프를 돌며 다시 시도
            continue;
        } else {
            cout << "bind() error" << endl;
            return 0;
        }
    }

    // 논블로킹 소켓은 listen()에서도 루프를 돌면서 될 때까지 계속 시도해야함
    while (listen(servsock, SOMAXCONN) == SOCKET_ERROR) {
        if (WSAGetLastError() == WSAEWOULDBLOCK) {
            // 논블로킹 소켓에서는 루프를 돌며 다시 시도
            continue;
        } else {
            cout << "listen() error" << endl;
            return 0;
        }
    }

    // 클라이언트 연결 대기
    while (true) {
        SOCKADDR_IN cliaddr;
        int addrlen = sizeof(cliaddr);
        SOCKET clisock = accept(servsock, (SOCKADDR*)&cliaddr, &addrlen);
        //클라이언트 연결 요청 수락
        if (clisock == INVALID_SOCKET) {
            if (WSAGetLastError() == WSAEWOULDBLOCK) {
                // 논블로킹 소켓에서는 루프를 돌며 다시 시도
                continue;
            } else { 
                cout << "accept() error" << endl;
                return 0;
            }
        }

        // 클라이언트 연결 성공
        cout << "Client Connected" << endl;

        // 클라이언트 요청 읽기 버퍼 생성
        char buf[1024] = "";

        // 클라이언트 요청 읽기 (정상적인 값이 들어올 때까지 루프)
        int recvlen = 0;
        // 클라이언트 요청을 저장할 변수
        string request = "";
        while(true){
            recvlen = recv(clisock, buf, sizeof(buf), 0);
            if (recvlen == SOCKET_ERROR) { // 에러 발생
                // 논블로킹 소켓은 revc()에서도 루프를 돌면서 될 때까지 계속 시도해야함
                if (WSAGetLastError() == WSAEWOULDBLOCK) {
                    continue;
                } else {
                    cout << "recv() error" << endl;
                    break;
                }
            } else if (recvlen == 0) { // 클라이언트 연결 종료
                cout << "Client Disconnected" << endl;
                closesocket(clisock);
                break; // 클라이언트 연결 종료 시 루프 종료
            } else {
                // 클라이언트 요청을 request에 추가
                request += string(buf, recvlen);
                // 정상적인 요청인지 확인
                if (request.find("\r\n\r\n") != string::npos) {
                    // 정상적인 요청이 완료된 경우 응답을 보내고 루프 종료
                    break;
                }
            }
        }
        
        // 클라이언트의 요청이 있을 경우
        if (!request.empty()) {
            // 클라이언트 요청 출력
            cout << "Request: " << request << endl;

            string response = "";
            if(strstr(request.c_str(), "GET / HTTP/1.1") != NULL) {
                response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
                response += readFileToString("index.html");
            } else if(strstr(request.c_str(), "GET /Find HTTP/1.1") != NULL) {
                response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
                response += readFileToString("Find.html");
            }
            else if(strstr(request.c_str(), "GET /about HTTP/1.1") != NULL) {
                response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
                response += readFileToString("about.html");
            }
            else if(strstr(request.c_str(), "GET /Goku HTTP/1.1") != NULL) {
                response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
                response += readFileToString("Goku.html");
            }
            else if(strstr(request.c_str(), "GET /Vegeta HTTP/1.1") != NULL) {
                response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
                response += readFileToString("Vegeta.html");
            }
            else {
                response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n";
                response += readFileToString("404.html");
            }
            send(clisock, response.c_str(), response.length(), 0);
        }
        
        // 클라이언트 연결 닫기
        closesocket(clisock);
        cout << "Client Disconnected" << endl;
    }

    closesocket(servsock); // 서버 소켓 닫기

    WSACleanup();
    return 0;
}
