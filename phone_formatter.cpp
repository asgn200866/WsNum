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

// Простая иконка (используем стандартную)
HICON LoadDefaultIcon() {
    return (HICON)LoadImage(GetModuleHandle(nullptr), IDI_APPLICATION,
                            IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
}

// Регистрация иконки в трее
void AddTrayIcon(HWND hwnd) {
    NOTIFYICONDATA nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadDefaultIcon();
    wcscpy_s(nid.szTip, L"Phone Formatter — Alt+V для форматирования");

    Shell_NotifyIcon(NIM_ADD, &nid);
}

// Удаление иконки из трея
void RemoveTrayIcon(HWND hwnd) {
    NOTIFYICONDATA nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    Shell_NotifyIcon(NIM_DELETE, &nid);
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
        MessageBox(g_hwndMain, L"Буфер обмена пуст!", L"Phone Formatter", MB_ICONWARNING | MB_OK);
        return;
    }

    std::wstring formatted = FormatPhoneNumber(original);
    if (formatted.empty()) {
        MessageBox(g_hwndMain,
            L"Не удалось распознать номер.\nПоддерживаются форматы:\n• +7 XXX XXX-XX-XX\n• 8XXX... \n• 79XXXXXXXXX",
            L"Ошибка формата", MB_ICONERROR | MB_OK);
        return;
    }

    if (!SetClipboardTextW(formatted)) {
        MessageBox(g_hwndMain, L"Не удалось изменить буфер обмена!", L"Ошибка", MB_ICONERROR);
        return;
    }

    SimulateCtrlV();
}

// Обработка кликов по иконке в трее
void OnTrayIconClick(WPARAM wParam, LPARAM lParam) {
    if (LOWORD(lParam) == WM_RBUTTONUP) {
        // Показываем контекстное меню
        POINT pt;
        GetCursorPos(&pt);

        HMENU hMenu = CreatePopupMenu();
        AppendMenuW(hMenu, MF_STRING, 1001, L"Выход");
        SetForegroundWindow(g_hwndMain); // чтобы меню закрылось при выборе
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
                MessageBox(hwnd, L"Программа уже запущена!", L"Ошибка", MB_ICONERROR);
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
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

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}