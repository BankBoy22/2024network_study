#include <iostream>
#include <WinSock2.h>
#include <thread>
#include <vector>
#include <string>
#include <sstream>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

constexpr int PORT = 12345;         // 포트번호는 12345
constexpr int MAX_CLIENTS = 2;      // 최대 클라이언트 수는 2명 (1vs1 대전을 생각하였기에)
constexpr int BUFFER_SIZE = 1024;

struct PlayerInfo {
    float x;
    float y;
    bool isAttacking;
    bool isHit;
    int health;
    bool isRolling;
};

struct ClientData {
    sockaddr_in address;
    PlayerInfo playerInfo;
    int playerNumber;
    bool isAlive;
};

std::vector<ClientData> clients;
std::mutex clientsMutex;
bool gameStarted = false;

// 플레이어의 상태를 클라이언트에게 브로드캐스팅하는 함수
void BroadcastPlayerState(const PlayerInfo& playerInfo, SOCKET serverSocket = INVALID_SOCKET) {
    std::string message = "PlayerState|" +
                          std::to_string(playerInfo.x) + "|" +
                          std::to_string(playerInfo.y) + "|" +
                          (playerInfo.isAttacking ? "1" : "0") + "|" +
                          (playerInfo.isHit ? "1" : "0") + "|" +
                          std::to_string(playerInfo.health) + "|" +
                          (playerInfo.isRolling ? "1" : "0");

    std::lock_guard<std::mutex> lock(clientsMutex);
    for (const auto& client : clients) {
        sendto(serverSocket, message.c_str(), message.size(), 0, reinterpret_cast<const sockaddr*>(&client.address), sizeof(client.address));
    }
}

std::vector<std::string> SplitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

// 클라이언트 핸들링 함수 (UDP) (핸들링은 멀티스레딩으로 처리)
void ClientHandler(SOCKET serverSocket) {
    char buffer[BUFFER_SIZE];
    int bytesReceived;
    sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);

    while (true) {
        bytesReceived = recvfrom(serverSocket, buffer, BUFFER_SIZE, 0, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrSize);
        if (bytesReceived == SOCKET_ERROR || bytesReceived == 0) {
            std::cerr << "Client disconnected\n";
            break; // 스레드를 종료하고 나감
        }

        buffer[bytesReceived] = '\0';
        std::cout << "Received from client: " << buffer << std::endl;

        std::string message(buffer);

        // 플레이어의 위치 메시지 처리
        if (message.find("Player1Position") != std::string::npos || message.find("Player2Position") != std::string::npos) {
            std::cout << "Received position update: " << message << std::endl;
            // 위치 정보 추출 (토큰화를 하면 플레이어 번호와 위치 정보가 나눠짐)
            std::vector<std::string> tokens = SplitString(message, '|');
            if (tokens.size() == 2) { // 메시지가 올바른 형식인지 확인
                // 플레이어 번호 추출 (1p 인지 2p 인지 확인하기 위함)
                int playerNumber = (message[6] == '1') ? 1 : 2;
                // 위치 정보 추출
                std::string positionInfo = tokens[1];
                // 모든 클라이언트에게 위치 정보 브로드캐스팅 (플레이어 번호와 위치 정보를 합쳐서 보냄)
                std::lock_guard<std::mutex> lock(clientsMutex);
                for (const auto& client : clients) {
                    sendto(serverSocket, buffer, bytesReceived, 0, reinterpret_cast<const sockaddr*>(&client.address), sizeof(client.address));
                }
            }
        }
        // 클라이언트의 flip 메시지 처리 find로 Player1Flipped 형식으로
        else if (message.find("Player1Flipped") != std::string::npos || message.find("Player2Flipped") != std::string::npos) {
            // 모든 클라이언트에게 flip 메시지 브로드캐스팅
            std::lock_guard<std::mutex> lock(clientsMutex);
            for (const auto& client : clients) {
                sendto(serverSocket, buffer, bytesReceived, 0, reinterpret_cast<const sockaddr*>(&client.address), sizeof(client.address));
            }
        }

        // 클라이언트로부터 받은 메시지를 처리
        else if (message.substr(0, 12) == "PlayerUpdate") {
            std::vector<std::string> tokens = SplitString(message, '|');
            if (tokens.size() == 7) {
                PlayerInfo playerInfo;
                playerInfo.x = std::stof(tokens[1]);
                playerInfo.y = std::stof(tokens[2]);
                playerInfo.isAttacking = tokens[3] == "1";
                playerInfo.isHit = tokens[4] == "1";
                playerInfo.health = std::stoi(tokens[5]);
                playerInfo.isRolling = tokens[6] == "1";

                BroadcastPlayerState(playerInfo, serverSocket);
            }
        }
        // 클라이언트가 죽었음을 알림 
        else if (message.substr(0, 10) == "Player1Dead" || message.substr(0, 10) == "Player2Dead") {
            // 게임 종료
            gameStarted = false;
            std::string endGameMessage = "EndGame";
            std::lock_guard<std::mutex> lock(clientsMutex);
            for (const auto& client : clients) {
                sendto(serverSocket, endGameMessage.c_str(), endGameMessage.size(), 0, reinterpret_cast<const sockaddr*>(&client.address), sizeof(client.address));
            }
        }
        else if (message.substr(0, 11) == "GameStarted") {
            gameStarted = true;
        }
        // 게임이 종료되었음을 알림 (시간이 종료되어서 끝난 경우)
        else if (message.substr(0, 8) == "GameOver") {
            gameStarted = false;
        }
    }
}

// 클라이언트에게 환영 메시지를 보내는 함수
void SendWelcomeMessage(const sockaddr_in& clientAddr, SOCKET serverSocket) {
    std::string welcomeMessage = "Welcome to the game server!";
    sendto(serverSocket, welcomeMessage.c_str(), welcomeMessage.size(), 0, reinterpret_cast<const sockaddr*>(&clientAddr), sizeof(clientAddr));
}

// 클라이언트에게 플레이어 번호를 할당하고, 게임 시작 여부를 확인하는 함수
void AssignPlayerNumber(const sockaddr_in& clientAddr, SOCKET serverSocket) {
    // 클라이언트가 연결되면 클라이언트의 수를 증가시키고, 클라이언트에게 플레이어 번호를 할당
    std::lock_guard<std::mutex> lock(clientsMutex);
    // 클라이언트의 수가 최대 클라이언트 수보다 작을 때만 클라이언트를 추가
    if (clients.size() < MAX_CLIENTS) {
        // 이미 연결된 클라이언트인지 확인
        bool alreadyConnected = false;
        for (const auto& client : clients) {
            if (client.address.sin_addr.s_addr == clientAddr.sin_addr.s_addr && client.address.sin_port == clientAddr.sin_port) {
                alreadyConnected = true;
                break;
            }
        }
        if (!alreadyConnected) {
            int playerNumber = clients.size() + 1;
            ClientData clientData;
            clientData.address = clientAddr;
            clientData.playerNumber = playerNumber;
            clientData.isAlive = true;  // 새로운 클라이언트는 살아있음
            clients.push_back(clientData);

            // 클라이언트의 데이터 한번 출력
            std::cout << clients.size() << " clients connected\n";

            // 환영 메시지 보내기
            SendWelcomeMessage(clientAddr, serverSocket);

            // 플레이어 번호 메시지 보내기
            std::string playerNumberMessage = std::to_string(playerNumber);
            sendto(serverSocket, playerNumberMessage.c_str(), playerNumberMessage.size(), 0, reinterpret_cast<const sockaddr*>(&clientAddr), sizeof(clientAddr));

            std::cout << "Assigned player number " << playerNumber << " to client\n";

            // 게임 시작 여부를 확인하고, 두 명의 클라이언트가 연결되었을 경우 게임 시작
            if (clients.size() == MAX_CLIENTS && !gameStarted) {
                std::string startGameMessage = "StartGame";
                for (const auto& client : clients) {
                    sendto(serverSocket, startGameMessage.c_str(), startGameMessage.size(), 0, reinterpret_cast<const sockaddr*>(&client.address), sizeof(client.address));
                }
                std::thread clientHandlerThread(ClientHandler, serverSocket);
                clientHandlerThread.detach(); // detach() 호출하여 메인 스레드가 클라이언트 핸들러 스레드를 기다리지 않도록 함
            }
        }
    }
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock\n";
        return -1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket\n";
        WSACleanup();
        return -1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Failed to bind\n";
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    std::cout << "Server started. Waiting for clients...\n";

    while (true) {
        char buffer[BUFFER_SIZE];
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);

        int bytesReceived = recvfrom(serverSocket, buffer, BUFFER_SIZE, 0, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrSize);
        if (bytesReceived == SOCKET_ERROR || bytesReceived == 0) {
            std::cerr << "Failed to receive connection request from client\n";
            continue;
        }

        AssignPlayerNumber(clientAddr, serverSocket);

        if (clients.size() >= MAX_CLIENTS) {
            // 최대 클라이언트 수에 도달하면 서버를 종료하지 않고 계속 대기합니다.
            std::cout << "Maximum number of clients reached. Waiting for more clients...\n";
        }
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
