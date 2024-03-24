/*

    CLIENT.CPP - Version 0.1

*/

#include <iostream>
#include <string>
#include <winsock2.h>
#include <windows.h>
#include "ascii.h"

#pragma comment(lib, "ws2_32.lib")

std::string username;

DWORD WINAPI receiveMessages(LPVOID lpParam) {
    SOCKET clientSocket = *((SOCKET*)lpParam);
    char buffer[1024];
    int bytesRead;

    while (true) {
        bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            std::cerr << "Server disconnected\n";
            break;
        }
        buffer[bytesRead] = '\0';

        if (strstr(buffer, "[SERVER]") != NULL) {
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            std::cout << buffer << std::endl;
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        } else {
            std::cout << buffer << std::endl;
        }
    }
    return 0;
}

int main() {
    printAsciiArt();
    WSADATA wsData;
    WORD ver = MAKEWORD(2, 2);

    if (WSAStartup(ver, &wsData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345); // Server Port
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // Server IP

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to server\n";

    std::cout << "Enter your username: ";
    std::getline(std::cin, username);

    send(clientSocket, username.c_str(), username.length(), 0);

    HANDLE recvThread = CreateThread(NULL, 0, receiveMessages, &clientSocket, 0, NULL);
    if (recvThread == NULL) {
        std::cerr << "Failed to create receive thread\n";
    } else {
        CloseHandle(recvThread);
    }

    std::string message;
    while (true) {
        std::getline(std::cin, message);
        if (message == "/quit") {
            break;
        }
        std::string fullMessage = "[" + username + "]: " + message;
        send(clientSocket, fullMessage.c_str(), fullMessage.length(), 0);
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}