# 4주차 과제 
## Unity 간단 게임 서버 연동
이 프로젝트는 Unity 클라이언트와 통신하여 멀티플레이어 게임을 구현하기 위한 서버 프로그램입니다.
### 개요 
이 프로젝트는 C++과 Winsock을 사용하여 작성되었습니다. 클라이언트와 서버 간의 통신은 UDP를 기반으로 하며, 클라이언트의 행동과 게임 상태를 동기화합니다.
### 기능 
최대 2명의 클라이언트까지 지원합니다.
클라이언트의 위치 및 행동 상태를 서버에 전달하고, 서버에서는 이 정보를 다른 클라이언트에 브로드캐스팅하여 동기화합니다.
게임 시작 및 종료 메시지를 처리하여 게임의 진행 상태를 관리합니다.
### 사용법
1. 서버를 실행하고 클라이언트의 연결을 대기합니다.
2. 최대 2명의 클라이언트가 연결되면 게임이 시작됩니다.
3. 클라이언트의 위치 및 행동을 확인하고, 게임의 종료 조건을 만족하면 게임을 종료합니다.
### 인게임 화면
![image](https://github.com/BankBoy22/2024network_study/assets/48702307/fa612e1e-a407-4c27-bb62-6aaebbcdac12)

실시간으로 서버에게 자신의 위치 포지션값을 전송
![image](https://github.com/BankBoy22/2024network_study/assets/48702307/58c6e662-b6b4-4701-ab68-83efb93f98dc)

1P 화면
![image](https://github.com/BankBoy22/2024network_study/assets/48702307/22fe9d72-aa9d-4fac-a141-85b49f099ef9)

2P 화면
![image](https://github.com/BankBoy22/2024network_study/assets/48702307/2ae34c37-fdc5-4b3c-b05c-78d79998f716)

실시간으로 상대의 위치와 Flip 값을 서버로 부터 전달 받아 화면에서 표시
![image](https://github.com/BankBoy22/2024network_study/assets/48702307/7fbbf95f-8628-4dba-a4e5-a697be2bbdfc)
