#include <windows.h>
#include <tchar.h>
#include <time.h>
#include <string>
#include <vector>

using namespace std;

#define ID_COPY 1001
#define ID_CUTOUT 1002
#define ID_DELETE 1003
#define ID_PASTE 1004
#define ID_FILL 1005
#define SELECT_ID 1006

HWND mainWindow;
HWND pathEdit;
HWND selectButton;
HHOOK hHook;
wstring filePath;   //путь файла у которого открыто контекстное меню

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
WNDPROC editWndProc;

bool selectActive = false;

wstring currentPath;  //текущая директрия
std::vector<wstring> selectedGroup; //буфер для всех путей к файлам

LPVOID buffer;

int cntSelected = 0; //кол-во выделенных файлов (применяется в удалении файлов)
bool cutOperation = false; //выделенные файлы необходимо при вставке перемещать 

char* bigbuffer = new char[2000000000]; //для работы с файлами




bool IsDirectory(wstring path)
{
    DWORD attributes = GetFileAttributes(path.c_str());
    return (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY));
}

void change_folder(wstring path) {

    if (path.back() == '\\') {
        path.pop_back();
    }

    EnumChildWindows(mainWindow, [](HWND hWndChild, LPARAM lParam) -> BOOL {   //удаление кнопок
        wchar_t className[256];
        GetClassName(hWndChild, className, 256);

        if (hWndChild == selectButton) { return TRUE; }

        if (wcscmp(className, L"Button") == 0) {
            DestroyWindow(hWndChild);
        }
        return TRUE;
        }, NULL);


    auto result = path;
    if (path.back() != '*') {
        result = result + L"\\*";
    }


    WIN32_FIND_DATA FindFileData;
    HANDLE hFind;
    hFind = FindFirstFile(result.c_str(), &FindFileData);
    int filecnt = 0;
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return;
    }
    do
    {
        if (FindFileData.cFileName != wstring(L".")) {
            if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) { //папка
                HWND hWnd = CreateWindow(L"BUTTON", FindFileData.cFileName, WS_VISIBLE | WS_CHILD | BS_LEFT | BS_CHECKBOX | WS_BORDER, 10, 40 + filecnt * 20, 600, 20, mainWindow, NULL, (HINSTANCE)GetWindowLongPtr(mainWindow, GWLP_HINSTANCE), NULL);
                filecnt++;
            }
        }
    } while (FindNextFile(hFind, &FindFileData));

    hFind = FindFirstFile(result.c_str(), &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return;
    }
    do
    {
        if (FindFileData.cFileName != wstring(L".")) {
            if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) { // файл
                HWND hWnd = CreateWindow(L"BUTTON", FindFileData.cFileName, WS_VISIBLE | WS_CHILD | BS_LEFT | BS_FLAT | BS_RADIOBUTTON | WS_BORDER, 10, 40 + filecnt * 20, 600, 20, mainWindow, NULL, (HINSTANCE)GetWindowLongPtr(mainWindow, GWLP_HINSTANCE), NULL);
                filecnt++;
            }
        }

    } while (FindNextFile(hFind, &FindFileData));

    currentPath = path;

    FindClose(hFind);

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

    const int window_x = 800;
    const int window_y = 500;




    // Window creation
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"LW3",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, window_x, window_y,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL)
    {
        return 0;
    }

    SetWindowLongPtr(hwnd, GWL_STYLE, GetWindowLongPtr(hwnd, GWL_STYLE) | WS_EX_ACCEPTFILES); //устанвка кодировки UTF-8

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

LRESULT CALLBACK editProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_KEYDOWN: {
        if (wParam == VK_RETURN)
        {
            wchar_t buffer[2048];
            GetWindowTextW((HWND)pathEdit, buffer, sizeof(buffer));
            wstring path = wstring(buffer);
            change_folder(path);
            return 0;
        }
    }
    default:
        return CallWindowProc(editWndProc, hwnd, uMsg, wParam, lParam);      //вызывает предыдущий обработчик для корректной отрисовки
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);

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
        FillRect(hdc, &ps.rcPaint, brush);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_CREATE:
    {
        mainWindow = hwnd;
        HWND hEdit = CreateWindowEx(0,
            L"EDIT",
            NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
            10, 10, 740, 20,
            hwnd,
            NULL,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            NULL);
        editWndProc = (WNDPROC)SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)editProc);
        SetWindowText(hEdit, L"D:\\");
        pathEdit = hEdit;

        selectButton = CreateWindow(L"Button", L"Выделить", WS_VISIBLE | WS_CHILD | BS_LEFT | BS_CHECKBOX | WS_BORDER, 650, 40, 100, 20, mainWindow, (HMENU)SELECT_ID, (HINSTANCE)GetWindowLongPtr(mainWindow, GWLP_HINSTANCE), NULL);


        memset(bigbuffer, 'A', sizeof(2000000000)); // буфер для мусора в файлах

        change_folder(L"D:\\");
        return 0;
    }


    case WM_COMMAND: {

        if ((HWND)lParam == selectButton) {
            if (IsDlgButtonChecked(mainWindow, SELECT_ID)) {
                SendMessage(selectButton, BM_SETCHECK, BST_UNCHECKED, 0);
                selectActive = false;
            }
            else {
                SendMessage(selectButton, BM_SETCHECK, BST_CHECKED, 0);
                selectActive = true;

            }
            return 0;
        }

        if (wParam == ID_COPY) {                                                                                                //контекстное меню:копирование
            cutOperation = false;
            selectedGroup.clear();
            EnumChildWindows(mainWindow, [](HWND hWndChild, LPARAM lParam) -> BOOL {   //запись в буфер
                wchar_t className[256];
                GetClassName(hWndChild, className, 256);

                if (hWndChild == selectButton) { return TRUE; }

                if (wcscmp(className, L"Button") == 0) {
                    LRESULT checked = SendMessage(hWndChild, BM_GETCHECK, 0, 0);
                    if (checked == BST_CHECKED) {
                        wchar_t buffer[257]; // буфер для текста окна
                        GetWindowText(hWndChild, buffer, sizeof(buffer));
                        selectedGroup.push_back(currentPath + L"\\" + buffer);
                    }
                }
                return TRUE;
                }, NULL);

            if (selectedGroup.size() == 0) { //текущий выбранный файл в буфер, если нет выделенных
                selectedGroup.push_back(filePath);
            }
        }

        if (wParam == ID_DELETE) {                                                                                      //контекстное меню:удаление

            cntSelected = 0;

            EnumChildWindows(mainWindow, [](HWND hWndChild, LPARAM lParam) -> BOOL {
                wchar_t className[256];
                GetClassName(hWndChild, className, 256);

                if (hWndChild == selectButton) { return TRUE; }

                if (wcscmp(className, L"Button") == 0) {
                    LRESULT checked = SendMessage(hWndChild, BM_GETCHECK, 0, 0);


                    if (checked == BST_CHECKED) {
                        cntSelected++;
                        wchar_t buffer[256];
                        GetWindowText(hWndChild, buffer, sizeof(buffer));
                        wstring filePath = currentPath + L"\\" + buffer;
                        if (!IsDirectory(filePath)) {
                            DeleteFile(filePath.c_str());
                        }
                        else {  //синтаксис удаления непустых папок

                            wchar_t* pathWithNull = new wchar_t[filePath.length() + 2]; //строка пути должна иметь 2 нультерминирующих символа
                            wcscpy_s(pathWithNull, filePath.length() + 1, filePath.c_str());
                            pathWithNull[filePath.length()] = L'\0';
                            pathWithNull[filePath.length() + 1] = L'\0';

                            SHFILEOPSTRUCT file_op = { NULL, FO_DELETE, pathWithNull , L"", FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT, false, 0, L"" };    //удаление без предупреждений
                            SHFileOperation(&file_op);
                        }
                    }

                }
                return TRUE;
                }, NULL);

            if (cntSelected == 0) {
                if (!IsDirectory(filePath)) {
                    DeleteFile(filePath.c_str());
                }
                else {  //синтаксис удаления непустых папок

                    wchar_t* pathWithNull = new wchar_t[filePath.length() + 2]; //строка пути должна иметь 2 нультерминирующих символа
                    wcscpy_s(pathWithNull, filePath.length() + 1, filePath.c_str());
                    pathWithNull[filePath.length()] = L'\0';
                    pathWithNull[filePath.length() + 1] = L'\0';

                    SHFILEOPSTRUCT file_op = { NULL, FO_DELETE, pathWithNull , L"", FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT, false, 0, L"" };    //удаление без предупреждений
                    SHFileOperation(&file_op);
                }
            }
            change_folder(currentPath);
        }

        if (wParam == ID_CUTOUT) {                                                                                                   //контекстное меню:вырезка
            cutOperation = true;
            selectedGroup.clear();
            EnumChildWindows(mainWindow, [](HWND hWndChild, LPARAM lParam) -> BOOL {  //запись в буфер
                wchar_t className[256];
                GetClassName(hWndChild, className, 256);

                if (hWndChild == selectButton) { return TRUE; }

                if (wcscmp(className, L"Button") == 0) {
                    LRESULT checked = SendMessage(hWndChild, BM_GETCHECK, 0, 0);
                    if (checked == BST_CHECKED) {
                        wchar_t buffer[257]; // буфер для текста окна
                        GetWindowText(hWndChild, buffer, sizeof(buffer));
                        selectedGroup.push_back(currentPath + L"\\" + buffer);
                    }
                }
                return TRUE;
                }, NULL);

            if (selectedGroup.size() == 0) { //текущий выбранный файл в буфер, если нет выделенных
                selectedGroup.push_back(filePath);
            }
        }

        if (wParam == ID_PASTE) {                                                                                                               //контекстное меню:вставка

            for (auto filePath : selectedGroup) {
                size_t pos = filePath.find_last_of(L"\\");
                auto filename = filePath.substr(pos + 1);
                wstring nextPath = currentPath + L"\\" + filename;

                if (!IsDirectory(filePath)) {
                    int num = 2;
                    if (GetFileAttributes(nextPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
                        nextPath = currentPath + L"\\" + to_wstring(1) + filename;
                    }
                    while (GetFileAttributes(nextPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
                        nextPath = currentPath + L"\\" + to_wstring(num) + filename;
                        num++;
                    }
                    CopyFile(filePath.c_str(), nextPath.c_str(), TRUE);    //true на перезапись файлов при копировании, не имеет значения
                }
                else {  //синтаксис копирования непустых папок

                    int num = 2;
                    if (GetFileAttributes(nextPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
                        nextPath = currentPath + L"\\" + to_wstring(1) + filename;
                    }
                    while (GetFileAttributes(nextPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
                        nextPath = currentPath + L"\\" + to_wstring(num) + filename;
                        num++;
                    }

                    wchar_t* fromPathWithNull = new wchar_t[filePath.length() + 2]; //строка пути должна иметь 2 нультерминирующих символа
                    wcscpy_s(fromPathWithNull, filePath.length() + 1, filePath.c_str());
                    fromPathWithNull[filePath.length()] = L'\0';
                    fromPathWithNull[filePath.length() + 1] = L'\0';

                    wchar_t* toPathWithNull = new wchar_t[nextPath.length() + 2]; //строка пути должна иметь 2 нультерминирующих символа
                    wcscpy_s(toPathWithNull, nextPath.length() + 1, nextPath.c_str());
                    toPathWithNull[nextPath.length()] = L'\0';
                    toPathWithNull[nextPath.length() + 1] = L'\0';

                    SHFILEOPSTRUCT file_op = { NULL, FO_COPY, fromPathWithNull , toPathWithNull, FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_SILENT, false, 0, L"" };
                    SHFileOperation(&file_op);
                }

                if (cutOperation) { //удаление 
                    if (!IsDirectory(filePath)) {
                        DeleteFile(filePath.c_str());
                    }
                    else {  //синтаксис удаления непустых папок

                        wchar_t* pathWithNull = new wchar_t[filePath.length() + 2]; //строка пути должна иметь 2 нультерминирующих символа
                        wcscpy_s(pathWithNull, filePath.length() + 1, filePath.c_str());
                        pathWithNull[filePath.length()] = L'\0';
                        pathWithNull[filePath.length() + 1] = L'\0';

                        SHFILEOPSTRUCT file_op = { NULL, FO_DELETE, pathWithNull , L"", FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT, false, 0, L"" };    //удаление без предупреждений
                        SHFileOperation(&file_op);
                    }
                }

            }
            if (cutOperation) {
                selectedGroup.clear();
            }
            change_folder(currentPath);
        }


        if (wParam == ID_FILL) {                                                                                       //контекстное меню: асинхронное заполнение мусором

            auto currentFile = CreateFile(filePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED, NULL); //FILE_FLAG_OVERLAPPED отвечает за асинхронность (перекрывающий ввод)  

            OVERLAPPED overlapped = { 0 };
            overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

            WriteFileEx(currentFile, bigbuffer, 2000000000, &overlapped, NULL);

            CloseHandle(overlapped.hEvent);
            CloseHandle(currentFile);

        }


        if (HIWORD(wParam) == BN_CLICKED)           //нажатие на кнопку
        {
            if (!selectActive) {    //переход по директориям
                wchar_t buffer[2048];
                wchar_t folder[512];
                GetWindowTextW((HWND)lParam, folder, sizeof(folder));
                GetWindowTextW((HWND)pathEdit, buffer, sizeof(buffer));

                wstring buf_str = wstring(buffer);
                wstring fold_str = wstring(folder);
                wstring result;

                if (buf_str.back() == '\\') {
                    buf_str.pop_back();
                }

                if (fold_str == L".") {
                    return 0;
                }
                else if (fold_str == L"..") {
                    size_t pos = buf_str.find_last_of('\\');
                    result = buf_str.substr(0, pos);
                }

                else {
                    result = buf_str + L"\\" + fold_str;
                }
                if (!IsDirectory(result)) {
                    return 0;
                }

                SetWindowText((HWND)pathEdit, (LPWSTR)result.c_str());
                change_folder(result);
            }
            else {//выделение
                wchar_t buffer[256];
                GetWindowText((HWND)lParam, buffer, sizeof(buffer));
                if (wcscmp(buffer, L"..") == 0)
                {
                    return 0;
                }
                LRESULT checked = SendMessage((HWND)lParam, BM_GETCHECK, 0, 0);
                if (checked == BST_CHECKED) {
                    SendMessage((HWND)lParam, BM_SETCHECK, BST_UNCHECKED, 0);
                }
                else {
                    SendMessage((HWND)lParam, BM_SETCHECK, BST_CHECKED, 0);
                }

            }

        }

        return 0;
    }
    case WM_CONTEXTMENU:
    {

        POINT pt;
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);

        HWND menuTarget = WindowFromPoint(pt);

        if (menuTarget != mainWindow) {
            wchar_t filename[256];
            GetWindowText(menuTarget, filename, 256);
            filePath = currentPath + L"\\" + filename;
        }

        wchar_t buffer1[256];
        wchar_t buffer2[256];
        GetClassName(menuTarget, buffer1, sizeof(buffer1));
        GetWindowText(menuTarget, buffer2, sizeof(buffer2));


        if (wcscmp(buffer1, L"Button") == 0 && wcscmp(buffer2, L"..") == 0)
        {
            return 0;
        }
        HMENU hMenu = CreatePopupMenu();
        if (menuTarget == mainWindow) {
            if (selectedGroup.size() != 0)InsertMenu(hMenu, 0, MF_BYPOSITION | MF_STRING, ID_PASTE, L"Вставить");
            else InsertMenu(hMenu, 0, MF_BYPOSITION | MF_STRING | MF_GRAYED, ID_PASTE, L"Вставить");
        }
        else {
            InsertMenu(hMenu, 0, MF_BYPOSITION | MF_STRING, ID_COPY, L"Копировать");
            InsertMenu(hMenu, 1, MF_BYPOSITION | MF_STRING, ID_CUTOUT, L"Вырезать");
            InsertMenu(hMenu, 2, MF_BYPOSITION | MF_STRING, ID_DELETE, L"Удалить");
            if (!IsDirectory(filePath))InsertMenu(hMenu, 3, MF_BYPOSITION | MF_STRING, ID_FILL, L"Заполнить мусором");
        }

        TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN, LOWORD(lParam), HIWORD(lParam), 0, hwnd, NULL);

        DestroyMenu(hMenu);

        return 0;
    }

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
