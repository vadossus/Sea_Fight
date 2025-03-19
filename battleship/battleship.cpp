#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdlib>
#include <ctime>
#include <iomanip>

#pragma comment(lib, "ws2_32.lib")

#define PORT 12345
#define BOARD_SIZE 10

void initializeWinsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Ошибка инициализации WinSock" << std::endl;
        exit(1);
    }
}

SOCKET createSocket() {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Ошибка создания сокета" << std::endl;
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
        std::cerr << "Ошибка привязки сокета" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        exit(1);
    }

    if (listen(serverSocket, 1) == SOCKET_ERROR) {
        std::cerr << "Ошибка прослушивания порта" << std::endl;
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
        std::cerr << "Ошибка подключения клиента" << std::endl;
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
        std::cerr << "Ошибка преобразования IP-адреса" << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        exit(1);
    }

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Ошибка подключения к серверу" << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        exit(1);
    }
    return clientSocket;
}

void printBoard(char board[BOARD_SIZE][BOARD_SIZE]) {
    std::cout << "  ";
    for (int i = 0; i < BOARD_SIZE; i++) {
        std::cout << i << " ";
    }
    std::cout << std::endl;
    for (int i = 0; i < BOARD_SIZE; i++) {
        std::cout << i << " ";
        for (int j = 0; j < BOARD_SIZE; j++) {
            char cell = board[i][j];
            if (cell == 0) cell = '.'; 
            else if (cell == 'K') cell = 'K'; 
            std::cout << cell << " ";
        }
        std::cout << std::endl;
    }
}


bool canPlaceShip(char board[BOARD_SIZE][BOARD_SIZE], int x, int y, int size, bool horizontal) {
    if (horizontal) {
        if (y + size > BOARD_SIZE) return false;

        // Проверка самой области корабля и окружающих клеток
        for (int i = y - 1; i <= y + size; ++i) {
            for (int j = x - 1; j <= x + 1; ++j) {
                if (i >= 0 && i < BOARD_SIZE && j >= 0 && j < BOARD_SIZE) {
                    if (board[j][i] != 0) return false; // Если клетка занята, корабль нельзя разместить
                }
            }
        }
    }
    else {
        // Проверка, чтобы корабль не выходил за пределы доски
        if (x + size > BOARD_SIZE) return false;

        // Проверка самой области корабля и окружающих клеток
        for (int i = x - 1; i <= x + size; ++i) {
            for (int j = y - 1; j <= y + 1; ++j) {
                if (i >= 0 && i < BOARD_SIZE && j >= 0 && j < BOARD_SIZE) {
                    if (board[i][j] != 0) return false; // Если клетка занята, корабль нельзя разместить
                }
            }
        }
    }
    return true; // Корабль можно разместить
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


int customRand(int max, unsigned int additionalSeed) {
    return (std::rand() ^ additionalSeed) % max;
}

void setupShips(char board[BOARD_SIZE][BOARD_SIZE], unsigned int seed) {
    srand(static_cast<unsigned int>(time(0)) + seed);

    // Очистка доски
    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            board[i][j] = 0;
        }
    }

    // Размеры кораблей (пример для классического морского боя)
    int shipSizes[] = { 4, 3, 3, 2, 2, 2, 1, 1, 1, 1 };
    for (int size : shipSizes) {
        bool placed = false;
        int attempts = 0; // Счетчик попыток размещения
        while (!placed && attempts < 1000) { // Ограничим количество попыток
            bool horizontal = rand() % 2 == 0; // Случайно выбираем ориентацию корабля
            int x, y;

            if (horizontal) {
                // Горизонтальный корабль: выравниваем по верхнему краю (y = 0)
                x = rand() % (BOARD_SIZE - size + 1); // Случайная начальная точка x
                y = 0; // Корабль начинается в верхнем ряду
            }
            else {
                // Вертикальный корабль: выравниваем по левому краю (x = 0)
                x = 0; // Корабль начинается в левом столбце
                y = rand() % (BOARD_SIZE - size + 1); // Случайная начальная точка y
            }

            // Проверка, можно ли разместить корабль
            if (canPlaceShip(board, x, y, size, horizontal)) {
                placeShip(board, x, y, size, horizontal);
                placed = true;
            }
            attempts++;
        }
        if (!placed) {
            std::cerr << "Не удалось разместить корабль размером " << size << std::endl;
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

    unsigned int additionalSeed = isHost ? 12345 : 54321;
    setupShips(myBoard, additionalSeed);

    while (isRunning) {
        system("cls");
        std::cout << "Ваше поле:" << std::endl;
        printBoard(myBoard);
        std::cout << "Поле противника:" << std::endl;
        printBoard(enemyBoard);

        bool turn = true;
        while (turn) {
            if (isHost) {
                std::cout << "Ваш ход (x y): ";
                std::cin >> x >> y;

                if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE) {
                    std::cout << "Некорректные координаты. Попробуйте снова." << std::endl;
                    continue;
                }

                sprintf_s(buffer, "%d %d", x, y);
                send(socket, buffer, sizeof(buffer), 0);

                recv(socket, buffer, sizeof(buffer), 0);
                char result = buffer[0];

                if (result == 'X') {
                    std::cout << "Попадание! Вы ходите снова." << std::endl;
                    enemyBoard[x][y] = 'X';
                    enemyShips--;
                    if (enemyShips == 0) {
                        std::cout << "Поздравляем! Вы победили!" << std::endl;
                        isRunning = false;
                        break;
                    }
                }
                else {
                    std::cout << "Промах!" << std::endl;
                    enemyBoard[x][y] = 'O';
                    turn = false;
                }
            }
            else {
                std::cout << "Ожидание хода противника..." << std::endl;
                recv(socket, buffer, sizeof(buffer), 0);
                sscanf_s(buffer, "%d %d", &x, &y);
                std::cout << "Противник выстрелил в: " << x << " " << y << std::endl;

                char result = 'O';
                if (myBoard[x][y] == 'S') {
                    result = 'X';
                    myBoard[x][y] = 'X';
                    myShips--;
                    if (myShips == 0) {
                        std::cout << "Противник победил!" << std::endl;
                        isRunning = false;
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
    std::cout << "Выберите режим (h - хост, c - клиент): ";
    std::cin >> choice;

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
        std::cout << "Введите IP сервера: ";
        std::cin >> serverIP;
        SOCKET clientSocket = connectToServer(serverIP);
        playGame(clientSocket, false);
        closesocket(clientSocket);
    }

    WSACleanup();
    return 0;
}
