/*

    SERVER.CPP - Version 0.1

*/

#include <iostream>
#include <vector>
#include <algorithm>
#include <winsock2.h>
#include <windows.h>
#include <string>
#include "ascii.h"

#pragma comment(lib, "ws2_32.lib")

std::vector<SOCKET> clients;
CRITICAL_SECTION consoleCriticalSection;

void sendToAllClients(const std::string& message) {
    EnterCriticalSection(&consoleCriticalSection);
    for (SOCKET& client : clients) {
        send(client, message.c_str(), message.size(), 0);
    }
    LeaveCriticalSection(&consoleCriticalSection);
}

void printServerMessage(const std::string& message) {
    std::string formattedMessage = "[SERVER]: " + message;

    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_INTENSITY);
    std::cout << formattedMessage << std::endl;

    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    sendToAllClients(formattedMessage);
}

DWORD WINAPI clientHandler(LPVOID lpParam) {
    SOCKET clientSocket = *((SOCKET*)lpParam);
    char buffer[1024];
    int bytesRead;
    std::string username;

    bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        username = buffer;
        printServerMessage("User '" + username + "' connected");
    } else {
        std::cerr << "Failed to receive username\n";
        closesocket(clientSocket);
        return 1;
    }

    EnterCriticalSection(&consoleCriticalSection);
    clients.push_back(clientSocket);
    LeaveCriticalSection(&consoleCriticalSection);

    while (true) {
        bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            printServerMessage("User '" + username + "' disconnected");
            break;
        }
        buffer[bytesRead] = '\0';
        std::cout << "User '" << username << "': " << buffer << std::endl;

        sendToAllClients("[" + username + "]: " + buffer);
    }

    EnterCriticalSection(&consoleCriticalSection);
    clients.erase(std::remove(clients.begin(), clients.end(), clientSocket), clients.end());
    LeaveCriticalSection(&consoleCriticalSection);

    return 0;
}

void readMessagesFromConsole() {
    while (true) {
        std::string message;
        std::getline(std::cin, message);
        printServerMessage(message);
    }
}

int main() {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_INTENSITY);
    printAsciiArt();
    WSADATA wsData;
    WORD ver = MAKEWORD(2, 2);

    if (WSAStartup(ver, &wsData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Binding failed: " << WSAGetLastError() << "\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listening failed\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server started, waiting for connections...\n";

    InitializeCriticalSection(&consoleCriticalSection);

    HANDLE consoleThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)readMessagesFromConsole, NULL, 0, NULL);
    if (consoleThread == NULL) {
        std::cerr << "Failed to create console thread\n";
        DeleteCriticalSection(&consoleCriticalSection);
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Connection accepting failed\n";
            break;
        }


        HANDLE clientThread = CreateThread(NULL, 0, clientHandler, &clientSocket, 0, NULL);
        if (clientThread == NULL) {
            std::cerr << "Failed to create client thread\n";
            closesocket(clientSocket);
        } else {
            CloseHandle(clientThread);
        }
    }

    CloseHandle(consoleThread);
    DeleteCriticalSection(&consoleCriticalSection);
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}