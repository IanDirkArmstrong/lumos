// Lumos - System tray module
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#include "tray.h"
#include "../../../resources/resource.h"

namespace lumos::platform {

Tray::~Tray()
{
    destroy();
}

bool Tray::create(HWND parent, UINT callback_msg)
{
    if (created_) return true;

    hwnd_ = parent;
    callback_msg_ = callback_msg;

    nid_.cbSize = sizeof(NOTIFYICONDATAW);
    nid_.hWnd = hwnd_;
    nid_.uID = 1;
    nid_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid_.uCallbackMessage = callback_msg_;
    nid_.hIcon = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_LUMOS));
    wcscpy_s(nid_.szTip, L"Lumos - Gamma Control");

    if (!Shell_NotifyIconW(NIM_ADD, &nid_)) {
        return false;
    }

    created_ = true;
    return true;
}

void Tray::destroy()
{
    if (!created_) return;

    Shell_NotifyIconW(NIM_DELETE, &nid_);
    created_ = false;
}

void Tray::showMenu()
{
    POINT pt;
    GetCursorPos(&pt);

    HMENU menu = CreatePopupMenu();
    if (!menu) return;

    AppendMenuW(menu, MF_STRING, ID_OPEN, L"Open Lumos");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, ID_RESET, L"Reset Gamma");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, ID_HELP, L"Help");
    AppendMenuW(menu, MF_STRING, ID_ABOUT, L"About");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, ID_EXIT, L"Exit");

    // Required for menu to work properly from tray
    SetForegroundWindow(hwnd_);

    UINT cmd = TrackPopupMenu(
        menu,
        TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON,
        pt.x, pt.y,
        0, hwnd_, nullptr);

    DestroyMenu(menu);

    // Handle selection
    switch (cmd) {
    case ID_OPEN:
        if (on_open) on_open();
        break;
    case ID_RESET:
        if (on_reset) on_reset();
        break;
    case ID_HELP:
        if (on_help) on_help();
        break;
    case ID_ABOUT:
        if (on_about) on_about();
        break;
    case ID_EXIT:
        if (on_exit) on_exit();
        break;
    }

    // Required for menu to dismiss properly
    PostMessageW(hwnd_, WM_NULL, 0, 0);
}

bool Tray::handleMessage(WPARAM wParam, LPARAM lParam)
{
    (void)wParam;

    switch (LOWORD(lParam)) {
    case WM_RBUTTONUP:
        showMenu();
        return true;
    case WM_LBUTTONDBLCLK:
        if (on_open) on_open();
        return true;
    }

    return false;
}

} // namespace lumos::platform
