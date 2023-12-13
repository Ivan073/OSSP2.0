// необходимые подключения для корректной работы WinSock2
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

#define PORT 8888   //порт для сервера вебсокета

using namespace std;


string chat;


void ClientHandler(SOCKET ClientSocket) {   //считывает данные пользователя с сокета
    char buffer[1024];
    int bytes;

    send(ClientSocket, (char*)chat.c_str(), chat.size(), 0);
    string user_chat = chat;    //версия загруженная у пользователя


    srand(time(0));
    int client_id = rand();
    cout << "Создан новый клиент: "+client_id << endl;
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


void ClientUpdater(SOCKET ClientSocket) {   //отправляет данные пользователю, когда данные изменяются
    string user_chat = chat;    //версия загруженная у пользователя

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


    //запуск winsocket
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "Ошибка при инициализации Winsock" << endl;
        return 1;
    }

    SOCKET ListenSocket, ClientSocket;                //сокеты для клиента и сервера
    struct sockaddr_in serverAddress, clientAddress;  //адреса сервера и клиента
    int addrlen = sizeof(clientAddress);
    char buffer[1024];                               //буфер для передаваемых данных


    // Создание сокета сервера (прослушивающего)
    ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //IPv4, потоковая передача данных (нет разделения на пакеты), использование TCP
    if (ListenSocket == INVALID_SOCKET) {
        cout << "Ошибка при создании сокета" << endl;
        return 1;
    }

    // Установка адреса сервера (хранится в специальной структуре)
    serverAddress.sin_family = AF_INET; //IPv4
    serverAddress.sin_addr.s_addr = INADDR_ANY; // приваязка только к локальным адресам (для полноценной сети необходимо заменить на свой IP адрес)
    serverAddress.sin_port = htons(PORT); // использование порта 8888

    //привязка адреса сервера к сокету прослушивания
    if (bind(ListenSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        cout << "Ошибка при привязке сокета к адресу сервера: " << WSAGetLastError() << endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Прослушивание сокета
    listen(ListenSocket, SOMAXCONN);
    cout << "Server started"  << endl;

    chat = "Chat created\n";

    // Принятие входящих соединений
    while (true) {
        SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET) {
            cout << "Ошибка при приеме соединения" << endl;
            continue;
        }
       // send(ClientSocket, (char*)chat.c_str(), chat.size(), 0);

        // Создание нового потока для обработки клиента
        thread(ClientHandler, ClientSocket).detach();
        thread(ClientUpdater, ClientSocket).detach();
    }

    return 0;
}