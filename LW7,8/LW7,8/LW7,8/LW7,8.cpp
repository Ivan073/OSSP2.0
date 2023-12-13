#define _CRT_SECURE_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <websocket.h>
#include <winhttp.h> 

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "websocket.lib")
#pragma comment(lib, "winhttp.lib") 


#include <tchar.h>
#include <cstring>
#include <string>
#include <cstdlib>
#include <cwchar>
#include <iostream>
#include <thread>

using namespace std;

HWND mainWindow;
HWND hEdit;
HWND chatLabel;

SOCKET ConnectSocket = INVALID_SOCKET;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

auto scrollValue = HIWORD(GetDialogBaseUnits());
auto yScrollPos = 0;

void ServerHandler(SOCKET ClientSocket) {   //аргумент не используется, нужен из-за отсутствия перегрузки в библиотеке
    char buffer[1024];
    int bytes;
    while (true) {
        bytes = recv(ConnectSocket, buffer, sizeof(buffer), 0);
        if (bytes <=0) {
            continue;
        }
        wchar_t wdata[1024];
        size_t converted = 0;
        mbstowcs(wdata, buffer, 1024);
        std::wstring data(wdata, 1024);
        SetWindowText(chatLabel, data.c_str());
    }

    closesocket(ConnectSocket);
}


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{

    // Window class.
    const wchar_t CLASS_NAME[] = L"Window";
    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    const int window_x = 510;
    const int window_y = 300;

    // Window creation
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"LW7,8",
        WS_OVERLAPPEDWINDOW | WS_VSCROLL,
        CW_USEDEFAULT, CW_USEDEFAULT, window_x, window_y,
        NULL,
        NULL,
        hInstance,
        NULL
    );
    mainWindow = hwnd;


    auto connectButton = CreateWindow(
        L"BUTTON",
        L"Подключиться к серверу",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        20,
        20,
        450,
        20,
        hwnd,
        (HMENU)1, //id
        (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
        NULL);

    auto sendButton = CreateWindow(
        L"BUTTON",
        L">",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        450,
        45,
        20,
        20,
        hwnd,
        (HMENU)2,   //id
        (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
        NULL);

    hEdit = CreateWindowEx(0,
        L"EDIT",
        NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
        20, 45, 430, 20,
        hwnd,
        NULL,
        (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
        NULL);


    chatLabel = CreateWindowEx(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 20, 70, 450, 5000, hwnd, NULL, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

    if (hwnd == NULL)
    {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    // Message loop.
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY: {
        PostQuitMessage(0);
        return 0;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        HBRUSH brush = CreateSolidBrush(
            RGB(255, 255, 255)
        );
        FillRect(hdc, &ps.rcPaint, brush);  //фон

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_CREATE:
    {
        setlocale(LC_ALL, "");
        return 0;
    }

    case WM_VSCROLL: {    //обработка скролла
        int dir = 0;
        switch (LOWORD(wParam)) {
        case SB_LINEUP:
            yScrollPos -= scrollValue;
            dir = 1;
            break;
        case SB_LINEDOWN:
            yScrollPos += scrollValue;
            dir = -1;
            break;
        case SB_THUMBPOSITION: {
            yScrollPos = HIWORD(wParam);
            auto curPos = LOWORD(GetScrollPos(mainWindow, SB_VERT));
            SetScrollPos(mainWindow, SB_VERT, yScrollPos, TRUE);
            ScrollWindow(mainWindow, 0, scrollValue * (curPos - yScrollPos ), NULL, NULL);
            return 0;
        }
        default:
            break;
        }

        SetScrollPos(mainWindow, SB_VERT, yScrollPos, TRUE);
        ScrollWindow(mainWindow, 0, scrollValue * dir, NULL, NULL);

        return 0;
    }

    case WM_COMMAND: { //button click

        if (LOWORD(wParam) == 1){        //подключенние к серверу


            if (ConnectSocket != INVALID_SOCKET) {
                MessageBox(NULL, L"Подключение завершено", L"", MB_OK);
                closesocket(ConnectSocket);
                ConnectSocket = INVALID_SOCKET;
                return 0;
            }
            // Закрытие сокета

            //сокет для сервера
            struct addrinfo* result = NULL, //адрес сервера
                * ptr = NULL,
                hints;

                // Инициализация Winsock
                WSADATA wsaData;
                int iResult;
                iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
                if (iResult != 0) {
                    MessageBox(NULL, L"Ошибка при инициализации Winsock", L"Ошибка", MB_OK);
                    return 1;
                }

            ZeroMemory(&hints, sizeof(hints));
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;

            // Разрешение адреса сервера и порта
            iResult = getaddrinfo("localhost", "8888", &hints, &result);
            if (iResult != 0) {
                MessageBox(NULL, L"Ошибка при разрешении адреса сервера", L"Ошибка", MB_OK);
                WSACleanup();
                return 1;
            }

            // Попытка подключения к серверу
            for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

                // Создание сокета для подключения к серверу
                ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
                    ptr->ai_protocol);
                if (ConnectSocket == INVALID_SOCKET) {
                    MessageBox(NULL, L"Ошибка при создании сокета ", L"Ошибка", MB_OK);
                    WSACleanup();
                    return 1;
                }

                // Подключение к серверу
                iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
                if (iResult == SOCKET_ERROR) {  //неудачное подключение
                    closesocket(ConnectSocket);
                    ConnectSocket = INVALID_SOCKET;
                    continue;
                }
                break;
            }

            freeaddrinfo(result);

            if (ConnectSocket == INVALID_SOCKET) {
                MessageBox(NULL, L"Не удалось подключиться к серверу", L"Ошибка", MB_OK);
                WSACleanup();
                return 1;
            }

            // Отправка данных на сервер
            const wchar_t* wsendbuf = L"Connect request.\0";
            char sendbuf[1024];
            wcstombs(sendbuf, wsendbuf, 1024);
            iResult = send(ConnectSocket, sendbuf, 1024, 0);
            if (iResult == SOCKET_ERROR) {
                MessageBox(NULL, L"Ошибка при отправке данных на сервер", L"Ошибка", MB_OK);
                closesocket(ConnectSocket);
                WSACleanup();
                return 1;
            }

            // Получение данных от сервера
            char recvbuf[1024];
            iResult = recv(ConnectSocket, recvbuf, 1024, 0);

            wchar_t wdata[1024];
            size_t converted = 0;
            mbstowcs(wdata, recvbuf, 1024);
            std::wstring data(wdata, 1024);
            if (iResult > 0) {
            //    MessageBox(NULL, L"Соединение успешно", L"Данные получены", MB_OK);
              //  SetWindowText(chatLabel, data.c_str());
                thread(ServerHandler, ConnectSocket).detach();      //запуск цикла проверки сокета
            }
            else if (iResult == 0) {
                MessageBox(NULL, L"Соединение закрыто сервером", L"Ошибка", MB_OK);
            }
            else {
                MessageBox(NULL, L"Ошибка при получении данных от сервера", L"Ошибка", MB_OK);
            }

        }
        else if (LOWORD(wParam) == 2) {     //отправка данных
            if (ConnectSocket == INVALID_SOCKET) {
                MessageBox(NULL, L"Соединение не было установлено", L"Ошибка", MB_OK);
            }
            else {
                wchar_t wsendbuf[1024];
                char sendbuf[1024];
                GetWindowText((HWND)hEdit, wsendbuf, 1024);
                wcstombs(sendbuf, wsendbuf, 1024);

                auto iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf) + 1, 0);
            }
        }

        return 0;

    }

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}