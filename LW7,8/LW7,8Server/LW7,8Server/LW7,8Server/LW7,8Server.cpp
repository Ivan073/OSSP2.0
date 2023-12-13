// ����������� ����������� ��� ���������� ������ WinSock2
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>

#define PORT 8888   //���� ��� ������� ���������

using namespace std;


string chat;


void ClientHandler(SOCKET ClientSocket) {   //��������� ������ ������������ � ������
    char buffer[1024];
    int bytes;

    send(ClientSocket, (char*)chat.c_str(), chat.size(), 0);
    string user_chat = chat;    //������ ����������� � ������������


    srand(time(0));
    int client_id = rand();
    cout << "������ ����� ������: "+client_id << endl;
    while (true) {
        if (user_chat != chat) {
            send(ClientSocket, (char*)chat.c_str(), chat.size(), 0);
            user_chat = chat;
        }

        bytes = recv(ClientSocket, buffer, sizeof(buffer), 0);
        if (bytes == SOCKET_ERROR) {
            continue;
        }
        cout << "Client message " << client_id << ":" << buffer << endl <<"\0";
        chat += buffer; chat += "\n\0";
    }

    closesocket(ClientSocket);
}


void ClientUpdater(SOCKET ClientSocket) {   //���������� ������ ������������, ����� ������ ����������
    string user_chat = chat;    //������ ����������� � ������������

    while (true) {
        if (user_chat != chat) {
            send(ClientSocket, (char*)chat.c_str(), chat.size(), 0);
            user_chat = chat;
        }
    }

    closesocket(ClientSocket);
}

int main() {
    setlocale(0, "");


    //������ winsocket
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "������ ��� ������������� Winsock" << endl;
        return 1;
    }

    SOCKET ListenSocket, ClientSocket;                //������ ��� ������� � �������
    struct sockaddr_in serverAddress, clientAddress;  //������ ������� � �������
    int addrlen = sizeof(clientAddress);
    char buffer[1024];                               //����� ��� ������������ ������


    // �������� ������ ������� (���������������)
    ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //IPv4, ��������� �������� ������ (��� ���������� �� ������), ������������� TCP
    if (ListenSocket == INVALID_SOCKET) {
        cout << "������ ��� �������� ������" << endl;
        return 1;
    }

    // ��������� ������ ������� (�������� � ����������� ���������)
    serverAddress.sin_family = AF_INET; //IPv4
    serverAddress.sin_addr.s_addr = INADDR_ANY; // ��������� ������ � ��������� ������� (��� ����������� ���� ���������� �������� �� ���� IP �����)
    serverAddress.sin_port = htons(PORT); // ������������� ����� 8888

    //�������� ������ ������� � ������ �������������
    if (bind(ListenSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        cout << "������ ��� �������� ������ � ������ �������: " << WSAGetLastError() << endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // ������������� ������
    listen(ListenSocket, SOMAXCONN);
    cout << "Server started"  << endl;

    chat = "Chat created\n";

    // �������� �������� ����������
    while (true) {
        SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET) {
            cout << "������ ��� ������ ����������" << endl;
            continue;
        }
       // send(ClientSocket, (char*)chat.c_str(), chat.size(), 0);

        // �������� ������ ������ ��� ��������� �������
        thread(ClientHandler, ClientSocket).detach();
        thread(ClientUpdater, ClientSocket).detach();
    }

    return 0;
}