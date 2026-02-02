// Lumos - System tray module
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <shellapi.h>
#include <functional>

namespace lumos::platform {

class Tray {
public:
    Tray() = default;
    ~Tray();

    // Initialize tray icon
    bool create(HWND parent, UINT callback_msg);

    // Remove tray icon
    void destroy();

    // Show context menu at cursor position
    void showMenu();

    // Handle tray callback message, returns true if handled
    bool handleMessage(WPARAM wParam, LPARAM lParam);

    // Menu item callbacks
    std::function<void()> on_open;
    std::function<void()> on_reset;
    std::function<void()> on_help;
    std::function<void()> on_about;
    std::function<void()> on_close_to_tray;
    std::function<void()> on_exit;

    // Menu item IDs
    static constexpr UINT ID_OPEN = 1001;
    static constexpr UINT ID_RESET = 1002;
    static constexpr UINT ID_HELP = 1003;
    static constexpr UINT ID_ABOUT = 1004;
    static constexpr UINT ID_CLOSE_TO_TRAY = 1005;
    static constexpr UINT ID_EXIT = 1006;

private:
    HWND hwnd_ = nullptr;
    UINT callback_msg_ = 0;
    bool created_ = false;
    NOTIFYICONDATAW nid_{};
};

} // namespace lumos::platform
