#include <windows.h>
#include <tchar.h>
#include <time.h>

const int minecount = 10;
const int cCols = 10;
const int cRows = 10;
const int offset = 20;

int cellsLeft = cCols * cRows - minecount;
int flagcount = minecount;
int gamestate = 0;

HWND mainWindow;

int** getMinefield() {

    srand(time(NULL));
    int** mines = new int* [cCols];
    for (int i = 0; i < cCols; i++) {
        mines[i] = new int[cRows];
        for (int j = 0; j < cRows; j++) {
            mines[i][j] = 0;
        }
    }

    for (int i = 0; i < minecount; i++) {
        int val = rand() % (cRows * cCols);
        while (mines[val % cCols][val / cRows] == 1) {
            val = rand() % (cRows * cCols);
        }
        mines[val % cCols][val / cRows] = 1;
    }
    int test = mines[6][2];
    return mines;
}

HWND** getButtonfield() {

    HWND** buttons = new HWND * [cCols];
    for (int i = 0; i < cCols; i++) {
        buttons[i] = new HWND[cRows];
    }

    return buttons;
}

int** minefield = getMinefield();
HWND** buttonfield = getButtonfield();



LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK RBHookProc(int nCode, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{

    // Window class.
    const wchar_t CLASS_NAME[] = L"Window";
    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    const int window_x = 255;
    const int window_y = 280;

    // Window creation
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"LW1",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, window_x, window_y,
        NULL,
        NULL,
        hInstance,
        NULL
    );
    mainWindow = hwnd;

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
        int test = gamestate;

        FillRect(hdc, &ps.rcPaint, brush);  //фон

        brush = CreateSolidBrush(
            RGB(255, 255, 0)
        );
        SelectObject(hdc, brush);
        Ellipse(hdc, 110, 0, 130, 20);

        brush = CreateSolidBrush(
            RGB(0, 0, 0)
        );
        SelectObject(hdc, brush);
        Ellipse(hdc, 116, 6, 119, 9);
        Ellipse(hdc, 121, 6, 124, 9);


        if (gamestate == 0) {
            MoveToEx(hdc, 116, 12, NULL);
            LineTo(hdc, 124, 12);
        }

        if (gamestate == 1) {
            MoveToEx(hdc, 116, 12, NULL);
            AngleArc(hdc, 119, 8, 6, 240, 90);
        }

        if (gamestate == 2) {
            MoveToEx(hdc, 124, 14, NULL);
            AngleArc(hdc, 119, 18, 6, 60, 90);
        }

        wchar_t buffer[256];
        wsprintfW(buffer, L"F: %d", flagcount);

        RECT textRect = { 150, 0, 190, 20 };
        DrawText(hdc, buffer, -1, &textRect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

        for (int i = 0; i < cCols; i++)
        {
            for (int j = 0; j < cRows; j++) {
                RECT buttonRect = { offset + i * 20, offset + j * 20, offset + i * 20 + 20, offset + j * 20 + 20 };

                wchar_t buffer[256];
                if (minefield[i][j] == 1) {
                    brush = CreateSolidBrush(
                        RGB(0, 0, 0)
                    );
                    SelectObject(hdc, brush);
                    Ellipse(hdc, offset + i * 20 + 5, offset + j * 20 + 5, offset + i * 20 + 20 - 5, offset + j * 20 + 20 - 5);
                }
                else {
                    int cnt = 0;
                    if (i > 0 && j > 0 && minefield[i - 1][j - 1] == 1) { cnt++; }
                    if (i > 0 && minefield[i - 1][j] == 1) { cnt++; }
                    if (i > 0 && j < cRows - 1 && minefield[i - 1][j + 1] == 1) { cnt++; }
                    if (j > 0 && minefield[i][j - 1] == 1) { cnt++; }
                    if (j < cRows - 1 && minefield[i][j + 1] == 1) { cnt++; }
                    if (i < cCols - 1 && j > 0 && minefield[i + 1][j - 1] == 1) { cnt++; }
                    if (i < cCols - 1 && minefield[i + 1][j] == 1) { cnt++; }
                    if (i < cCols - 1 && j < cRows - 1 && minefield[i + 1][j + 1] == 1) { cnt++; }
                    wsprintfW(buffer, L"%d", cnt);
                    DrawText(hdc, buffer, -1, &buttonRect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
                }

            }
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_CREATE:
    {
        HHOOK hHook = SetWindowsHookEx(WH_MOUSE_LL, RBHookProc, NULL, 0);

        auto buttons = new HWND[cCols][cRows];
        for (int i = 0; i < cCols; i++)
        {
            for (int j = 0; j < cRows; j++) {
                buttonfield[i][j] = CreateWindow(
                    L"BUTTON",  // Button class
                    L"",      // Button text 
                    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
                    i * 20 + offset,         // x position 
                    j * 20 + offset,         // y position 
                    20,        // Button width
                    20,        // Button height
                    hwnd,     // Parent window
                    NULL,
                    (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                    NULL);
            }
        }
        return 0;
    }

    case WM_COMMAND: {

        if (!IsWindow((HWND)lParam)) {  //событытие для уже открытой плитки
            return 0;
        }

        if (wParam == WM_RBUTTONDOWN) { //нажатие правой кнопкой мыши


            TCHAR text[256];
            GetWindowText((HWND)lParam, text, 256);

            if (_tcslen(text) > 0) {
                SetWindowText((HWND)lParam, L"");
                flagcount++;
                RECT textRect = { 150, 0, 190, 20 };
                RedrawWindow(hwnd, &textRect, NULL, RDW_INVALIDATE | RDW_INTERNALPAINT);    //обновляется только часть экрана
            }
            else if (flagcount > 0) {
                SetWindowText((HWND)lParam, L"F");
                flagcount--;
                RECT textRect = { 150, 0, 190, 20 };
                RedrawWindow(hwnd, &textRect, NULL, RDW_INVALIDATE | RDW_INTERNALPAINT);
            }
            return 0;
        }

        TCHAR text[256];
        GetWindowText((HWND)lParam, text, 256);
        if (_tcslen(text) > 0) {    //кнопка с флагом
            return 0;
        }

        RECT buttonRect;
        RECT windowRect;
        GetWindowRect((HWND)lParam, &buttonRect);
        GetWindowRect((HWND)hwnd, &windowRect);
        int buttonX = (buttonRect.left - windowRect.left - 28) / 20;
        int buttonY = (buttonRect.top - windowRect.top - 51) / 20;

        DestroyWindow((HWND)lParam);

        int test = minefield[1][4];
        if (minefield[buttonX][buttonY] == 1) {     //проигрыш
            gamestate = 2;
            while (FindWindowEx(hwnd, NULL, NULL, NULL) != NULL)
            {
                auto hWndChild = FindWindowEx(hwnd, NULL, NULL, NULL);
                DestroyWindow(hWndChild);
            }
            RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_INTERNALPAINT);
        }
        else {
            cellsLeft--;
            if (cellsLeft == 0) {   //победа
                gamestate = 1;
                while (FindWindowEx(hwnd, NULL, NULL, NULL) != NULL)
                {
                    auto hWndChild = FindWindowEx(hwnd, NULL, NULL, NULL);
                    DestroyWindow(hWndChild);
                }
                RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_INTERNALPAINT);
            }
            int cnt = 0; // соседние мины
            int i = buttonX;
            int j = buttonY;
            int test = minefield[6][2];

            if (i > 0 && j > 0 && minefield[i - 1][j - 1] == 1) { cnt++; }
            if (i > 0 && minefield[i - 1][j] == 1) { cnt++; }
            if (i > 0 && j < cRows - 1 && minefield[i - 1][j + 1] == 1) { cnt++; }
            if (j > 0 && minefield[i][j - 1] == 1) { cnt++; }
            if (j < cRows - 1 && minefield[i][j + 1] == 1) { cnt++; }
            if (i < cCols - 1 && j > 0 && minefield[i + 1][j - 1] == 1) { cnt++; }
            if (i < cCols - 1 && minefield[i + 1][j] == 1) { cnt++; }
            if (i < cCols - 1 && j < cRows - 1 && minefield[i + 1][j + 1] == 1) { cnt++; }

            if (cnt == 0) { //открывать соседние
                if (i > 0 && j > 0) { SendMessage(hwnd, uMsg, wParam, (LPARAM)buttonfield[i - 1][j - 1]); }
                if (i > 0) { SendMessage(hwnd, uMsg, wParam, (LPARAM)buttonfield[i - 1][j]); }
                if (i > 0 && j < cRows - 1) { SendMessage(hwnd, uMsg, wParam, (LPARAM)buttonfield[i - 1][j + 1]); }
                if (j > 0) { SendMessage(hwnd, uMsg, wParam, (LPARAM)buttonfield[i][j - 1]); }
                if (j < cRows - 1) { SendMessage(hwnd, uMsg, wParam, (LPARAM)buttonfield[i][j + 1]); }
                if (i < cCols - 1 && j > 0) { SendMessage(hwnd, uMsg, wParam, (LPARAM)buttonfield[i + 1][j - 1]); }
                if (i < cCols - 1) { SendMessage(hwnd, uMsg, wParam, (LPARAM)buttonfield[i + 1][j]); }
                if (i < cCols - 1 && j < cRows - 1) { SendMessage(hwnd, uMsg, wParam, (LPARAM)buttonfield[i + 1][j + 1]); }
            }
        }
        return 0;
    }

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


LRESULT CALLBACK RBHookProc(int nCode, WPARAM wParam, LPARAM lParam)    //winhook для передачи события нажатия ПКМ
{
    if (wParam == WM_RBUTTONDOWN)
    {
        RECT windowRect;
        GetWindowRect(mainWindow, &windowRect);
        POINT pt;
        GetCursorPos(&pt);
        int mouseX = (pt.x - windowRect.left - 28) / 20;
        int mouseY = (pt.y - windowRect.top - 51) / 20;
        if((pt.x - windowRect.left - 28)>=0 && (pt.y - windowRect.top - 51)>=0 && mouseX>=0 && mouseX<cCols && mouseY >= 0 && mouseY < cRows)SendMessage(mainWindow, WM_COMMAND, wParam, (LPARAM)buttonfield[mouseX][mouseY]);
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}
