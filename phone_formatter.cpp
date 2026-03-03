// phone_formatter_tray.cpp
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <string>
#include <regex>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")

const UINT HOTKEY_ID = 1001;
const UINT WM_TRAYICON = WM_USER + 1;

HWND g_hwndMain = nullptr;
HICON g_hTrayIcon = nullptr;
HINSTANCE g_hInst = nullptr;

// Загрузка иконки для трея
HICON LoadTrayIcon() {
    HICON hIcon = nullptr;
    
    // Пытаемся загрузить иконку телефона из shell32.dll
    HMODULE hShell32 = LoadLibrary(L"shell32.dll");
    if (hShell32) {
        hIcon = (HICON)LoadImage(hShell32, MAKEINTRESOURCE(21), 
                                IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
        FreeLibrary(hShell32);
    }
    
    // Если не удалось, используем стандартную
    if (!hIcon) {
        hIcon = (HICON)LoadImage(GetModuleHandle(nullptr), IDI_APPLICATION,
                                IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    }
    
    return hIcon;
}

// Регистрация иконки в трее
void AddTrayIcon(HWND hwnd) {
    NOTIFYICONDATA nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    nid.uCallbackMessage = WM_TRAYICON;
    
    g_hTrayIcon = LoadTrayIcon();
    nid.hIcon = g_hTrayIcon;
    wcscpy_s(nid.szTip, L"Phone Formatter — Ctrl+D для форматирования");

    Shell_NotifyIcon(NIM_ADD, &nid);
}

// Удаление иконки из трея
void RemoveTrayIcon(HWND hwnd) {
    NOTIFYICONDATA nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    Shell_NotifyIcon(NIM_DELETE, &nid);
    
    if (g_hTrayIcon) {
        DestroyIcon(g_hTrayIcon);
        g_hTrayIcon = nullptr;
    }
}

// Оконная процедура для окна ошибки
LRESULT CALLBACK ErrorWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND:
            if (LOWORD(wParam) == 1) { // Кнопка OK
                DestroyWindow(hwnd);
                return 0;
            }
            break;
            
        case WM_KEYDOWN:
            if (wParam == VK_RETURN || wParam == VK_ESCAPE) {
                DestroyWindow(hwnd);
                return 0;
            }
            break;
            
        case WM_DESTROY:
            // Просто выходим, не завершая программу
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Показать окно с ошибкой (исправленная версия)
void ShowErrorWithFlash(const std::wstring& title, const std::wstring& message) {
    // Регистрируем класс для окна ошибки
    static bool registered = false;
    if (!registered) {
        WNDCLASSEX wcError = {0};
        wcError.cbSize = sizeof(WNDCLASSEX);
        wcError.lpfnWndProc = ErrorWndProc;
        wcError.hInstance = g_hInst;
        wcError.hIcon = LoadIcon(nullptr, IDI_ERROR);
        wcError.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcError.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wcError.lpszClassName = L"PhoneFormatterErrorClass";
        RegisterClassEx(&wcError);
        registered = true;
    }
    
    // Создаем окно
    HWND hwndError = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE,
        L"PhoneFormatterErrorClass",
        title.c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 450, 220,
        nullptr, nullptr, g_hInst, nullptr
    );
    
    if (hwndError) {
        // Создаем текст
        HWND hText = CreateWindowEx(0, L"EDIT", message.c_str(),
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | ES_LEFT | ES_AUTOVSCROLL,
            15, 15, 410, 120, hwndError, nullptr, g_hInst, nullptr);
        
        // Устанавливаем шрифт
        HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        SendMessage(hText, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        // Создаем кнопку OK
        HWND hButton = CreateWindowEx(0, L"BUTTON", L"OK",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
            175, 150, 100, 30, hwndError, (HMENU)1, g_hInst, nullptr);
        SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        // Мигаем окном
        FLASHWINFO flashInfo = {0};
        flashInfo.cbSize = sizeof(FLASHWINFO);
        flashInfo.hwnd = hwndError;
        flashInfo.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
        flashInfo.uCount = 0;
        flashInfo.dwTimeout = 0;
        FlashWindowEx(&flashInfo);
        
        // Центрируем
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        int x = (screenWidth - 450) / 2;
        int y = (screenHeight - 220) / 2;
        SetWindowPos(hwndError, HWND_TOPMOST, x, y, 450, 220, SWP_SHOWWINDOW);
        
        // Блокируем главное окно
        EnableWindow(g_hwndMain, FALSE);
        
        // Цикл сообщений для окна ошибки
        MSG msg = {};
        BOOL bRet;
        while ((bRet = GetMessage(&msg, nullptr, 0, 0)) != 0) {
            if (bRet == -1) break;
            
            if (!IsDialogMessage(hwndError, &msg)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            
            if (!IsWindow(hwndError)) break;
        }
        
        // Разблокируем главное окно
        EnableWindow(g_hwndMain, TRUE);
        SetForegroundWindow(g_hwndMain);
        
        // Очищаем ресурсы
        DeleteObject(hFont);
    }
}

// Форматирование номера
std::wstring FormatPhoneNumber(const std::wstring& input) {
    std::wregex non_digit(L"[^0-9]");
    std::wstring digits = std::regex_replace(input, non_digit, L"");

    if (digits.empty()) return L"";

    if (digits.length() == 11 && (digits[0] == L'7' || digits[0] == L'8')) {
        return digits.substr(1);
    }
    if (digits.length() == 10) {
        return digits;
    }
    return L"";
}

void SimulateCtrlV() {
    INPUT inputs[4] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_CONTROL;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = 'V';
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = 'V';
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_CONTROL;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(4, inputs, sizeof(INPUT));
    Sleep(50);
}

std::wstring GetClipboardTextW() {
    if (!OpenClipboard(nullptr)) return L"";
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (!hData) { CloseClipboard(); return L""; }
    wchar_t* pszText = static_cast<wchar_t*>(GlobalLock(hData));
    if (!pszText) { CloseClipboard(); return L""; }
    std::wstring text(pszText);
    GlobalUnlock(hData);
    CloseClipboard();
    return text;
}

bool SetClipboardTextW(const std::wstring& text) {
    if (!OpenClipboard(nullptr)) return false;
    EmptyClipboard();
    size_t size = (text.length() + 1) * sizeof(wchar_t);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!hMem) { CloseClipboard(); return false; }
    wchar_t* pszText = static_cast<wchar_t*>(GlobalLock(hMem));
    if (!pszText) { GlobalFree(hMem); CloseClipboard(); return false; }
    wcscpy_s(pszText, text.length() + 1, text.c_str());
    GlobalUnlock(hMem);
    SetClipboardData(CF_UNICODETEXT, hMem);
    CloseClipboard();
    return true;
}

void OnHotkeyPressed() {
    std::wstring original = GetClipboardTextW();
    if (original.empty()) {
        ShowErrorWithFlash(L"Phone Formatter - Ошибка", 
            L"Буфер обмена пуст!\n\nСкопируйте номер телефона и нажмите Ctrl+D снова.");
        return;
    }

    std::wstring formatted = FormatPhoneNumber(original);
    if (formatted.empty()) {
        ShowErrorWithFlash(L"Phone Formatter - Ошибка формата",
            L"Не удалось распознать номер.\n\nПоддерживаются форматы:\n"
            L"• +7 XXX XXX-XX-XX\n• 8XXX...\n• 79XXXXXXXXX\n\n"
            L"Скопируйте номер и нажмите Ctrl+D снова.");
        return;
    }

    if (!SetClipboardTextW(formatted)) {
        ShowErrorWithFlash(L"Phone Formatter - Ошибка",
            L"Не удалось изменить буфер обмена!\n\nПроверьте, не заблокирован ли буфер обмена другой программой.");
        return;
    }

    SimulateCtrlV();
}

// Обработка кликов по иконке в трее
void OnTrayIconClick(WPARAM wParam, LPARAM lParam) {
    if (LOWORD(lParam) == WM_RBUTTONUP) {
        POINT pt;
        GetCursorPos(&pt);

        HMENU hMenu = CreatePopupMenu();
        AppendMenuW(hMenu, MF_STRING, 1001, L"Выход");
        SetForegroundWindow(g_hwndMain);
        UINT cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, g_hwndMain, nullptr);
        DestroyMenu(hMenu);

        if (cmd == 1001) {
            PostQuitMessage(0);
        }
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            g_hwndMain = hwnd;
            if (!RegisterHotKey(hwnd, HOTKEY_ID, MOD_CONTROL, 'D')) {
                MessageBox(nullptr, L"Не удалось зарегистрировать горячую клавишу!", L"Ошибка", MB_ICONERROR);
            }
            AddTrayIcon(hwnd);
            break;

        case WM_HOTKEY:
            if (wParam == HOTKEY_ID) {
                OnHotkeyPressed();
            }
            break;

        case WM_TRAYICON:
            OnTrayIconClick(wParam, lParam);
            break;

        case WM_DESTROY:
            UnregisterHotKey(hwnd, HOTKEY_ID);
            RemoveTrayIcon(hwnd);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    g_hInst = hInstance;
    
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"PhoneFormatterTrayClass";
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassEx(&wc)) {
        MessageBox(nullptr, L"Ошибка регистрации класса!", L"Ошибка", MB_ICONERROR);
        return 1;
    }

    HWND hwnd = CreateWindowEx(
        0, L"PhoneFormatterTrayClass", L"Phone Formatter",
        0, 0, 0, 0, 0,
        HWND_MESSAGE, nullptr, hInstance, nullptr
    );

    if (!hwnd) {
        MessageBox(nullptr, L"Не удалось создать окно!", L"Ошибка", MB_ICONERROR);
        return 1;
    }

    // Показываем уведомление о запуске
    NOTIFYICONDATA nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_INFO;
    wcscpy_s(nid.szInfoTitle, L"Phone Formatter");
    wcscpy_s(nid.szInfo, L"Программа запущена. Используйте Ctrl+D для форматирования номера.");
    nid.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIcon(NIM_MODIFY, &nid);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}