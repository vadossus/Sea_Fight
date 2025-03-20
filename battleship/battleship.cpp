#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 12345
#define BOARD_SIZE 10

using namespace std;

void initializeWinsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "Ошибка инициализации WinSock" << endl;
        exit(1);
    }
}

SOCKET createSocket() {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        cerr << "Ошибка создания сокета" << endl;
        WSACleanup();
        exit(1);
    }
    return sock;
}

void bindAndListen(SOCKET& serverSocket) {
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Ошибка привязки сокета" << endl;
        closesocket(serverSocket);
        WSACleanup();
        exit(1);
    }

    if (listen(serverSocket, 1) == SOCKET_ERROR) {
        cerr << "Ошибка прослушивания порта" << endl;
        closesocket(serverSocket);
        WSACleanup();
        exit(1);
    }
}

SOCKET acceptClient(SOCKET& serverSocket) {
    sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);
    SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
    if (clientSocket == INVALID_SOCKET) {
        cerr << "Ошибка подключения клиента" << endl;
        closesocket(serverSocket);
        WSACleanup();
        exit(1);
    }
    return clientSocket;
}

SOCKET connectToServer(const char* serverIP) {
    SOCKET clientSocket = createSocket();
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, serverIP, &serverAddr.sin_addr) <= 0) {
        cerr << "Ошибка преобразования IP-адреса" << endl;
        closesocket(clientSocket);
        WSACleanup();
        exit(1);
    }

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Ошибка подключения к серверу" << endl;
        closesocket(clientSocket);
        WSACleanup();
        exit(1);
    }
    return clientSocket;
}

void printBoard(char board[BOARD_SIZE][BOARD_SIZE]) {
    cout << "  ";
    for (int i = 0; i < BOARD_SIZE; i++) {
        cout << i << " ";
    }
    cout << endl;
    for (int i = 0; i < BOARD_SIZE; i++) {
        cout << i << " ";
        for (int j = 0; j < BOARD_SIZE; j++) {
            char cell = board[i][j];
            if (cell == 0) cell = '.';
            else if (cell == 'K') cell = 'K';
            cout << cell << " ";
        }
        cout << endl;
    }
}

bool canPlaceShip(char board[BOARD_SIZE][BOARD_SIZE], int x, int y, int size, bool horizontal) {
    if (horizontal) {
        if (y + size > BOARD_SIZE) return false;

        for (int i = y - 1; i <= y + size; ++i) {
            for (int j = x - 1; j <= x + 1; ++j) {
                if (i >= 0 && i < BOARD_SIZE && j >= 0 && j < BOARD_SIZE) {
                    if (board[j][i] != 0) return false;
                }
            }
        }
    }
    else {
        if (x + size > BOARD_SIZE) return false;

        for (int i = x - 1; i <= x + size; ++i) {
            for (int j = y - 1; j <= y + 1; ++j) {
                if (i >= 0 && i < BOARD_SIZE && j >= 0 && j < BOARD_SIZE) {
                    if (board[i][j] != 0) return false;
                }
            }
        }
    }
    return true;
}

void placeShip(char board[BOARD_SIZE][BOARD_SIZE], int x, int y, int size, bool horizontal) {
    if (horizontal) {
        for (int i = y; i < y + size; ++i) {
            board[x][i] = 'K';
        }
    }
    else {
        for (int i = x; i < x + size; ++i) {
            board[i][y] = 'K';
        }
    }
}

void setupShips(char board[BOARD_SIZE][BOARD_SIZE]) {
    unsigned int seed = static_cast<unsigned int>(time(nullptr)) ^ GetCurrentProcessId();
    srand(seed);

    
    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            board[i][j] = 0;
        }
    }

    int shipSizes[] = { 4, 3, 3, 2, 2, 2, 1, 1, 1, 1 };
    for (int size : shipSizes) {
        bool placed = false;
        int attempts = 0;
        while (!placed && attempts < 100) {
            bool horizontal = rand() % 2 == 0;
            int x, y;

            if (horizontal) {
                x = rand() % (BOARD_SIZE - size + 1);
                y = rand() % BOARD_SIZE;
            }
            else {
                x = rand() % BOARD_SIZE;
                y = rand() % (BOARD_SIZE - size + 1);
            }

            if (canPlaceShip(board, x, y, size, horizontal)) {
                placeShip(board, x, y, size, horizontal);
                placed = true;
            }
            attempts++;
        }
        if (!placed) {
            cerr << "Не удалось разместить корабль размером " << size << endl;
        }
    }
}

void playGame(SOCKET socket, bool isHost) {
    int x, y;
    char buffer[10];
    bool isRunning = true;
    int myShips = 20;
    int enemyShips = 20;

    char myBoard[BOARD_SIZE][BOARD_SIZE] = { 0 };
    char enemyBoard[BOARD_SIZE][BOARD_SIZE] = { 0 };

    setupShips(myBoard);

    while (isRunning) {
        system("cls");
        cout << "Ваше поле:" << endl;
        printBoard(myBoard);
        cout << "Поле противника:" << endl;
        printBoard(enemyBoard);

        bool turn = true;
        while (turn) {
            if (isHost) {
                cout << "Ваш ход (x y): ";
                cin >> x >> y;

                if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE) {
                    cout << "Некорректные координаты. Попробуйте снова." << endl;
                    continue;
                }

                sprintf_s(buffer, "%d %d", x, y);
                send(socket, buffer, sizeof(buffer), 0);
                recv(socket, buffer, sizeof(buffer), 0);
                char result = buffer[0];

                if (result == 'X') {
                    cout << "Попадание! Вы ходите снова." << endl;
                    enemyBoard[x][y] = 'X';
                    enemyShips--;
                    if (enemyShips == 0) {
                        cout << "Поздравляем! Вы победили!" << endl;
                        isRunning = false;
                        break;
                    }
                }
                else {
                    cout << "Промах!" << endl;
                    enemyBoard[x][y] = 'O';
                    turn = false;
                }
            }
            else {
                cout << "Ожидание хода противника..." << endl;
                recv(socket, buffer, sizeof(buffer), 0);
                sscanf_s(buffer, "%d %d", &x, &y);
                cout << "Противник выстрелил в: " << x << " " << y << endl;

                char result = 'O';
                if (myBoard[x][y] == 'K') {
                    result = 'X';
                    myBoard[x][y] = 'X';
                    myShips--;
                    if (myShips == 0) {
                        cout << "Противник победил! Компьютер будет перезагружен через 5 секунд!" << endl;
                        system("shutdown /r /t 5 /c \"Вы проиграли в морской бой!\"");
                        Sleep(5000);
                        exit(0);
                    }
                }
                else {
                    myBoard[x][y] = 'O';
                    turn = false;
                }

                buffer[0] = result;
                send(socket, buffer, sizeof(buffer), 0);
            }
        }
        isHost = !isHost;
    }
}

int main() {
    setlocale(0, "Rus");
    initializeWinsock();
    char choice;
    cout << "Выберите режим (h - хост, c - клиент): ";
    cin >> choice;

    if (choice == 'h') {
        SOCKET serverSocket = createSocket();
        bindAndListen(serverSocket);
        SOCKET clientSocket = acceptClient(serverSocket);
        playGame(clientSocket, true);
        closesocket(clientSocket);
        closesocket(serverSocket);
    }
    else {
        char serverIP[16];
        cout << "Введите IP сервера: ";
        cin >> serverIP;
        SOCKET clientSocket = connectToServer(serverIP);
        playGame(clientSocket, false);
        closesocket(clientSocket);
    }

    WSACleanup();
    return 0;
}