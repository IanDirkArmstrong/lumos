// Lumos - Global hotkeys module
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <functional>

namespace lumos::platform {

class Hotkeys {
public:
    Hotkeys() = default;
    ~Hotkeys();

    // Initialize hotkeys (call after window creation)
    bool initialize(HWND hwnd);

    // Cleanup hotkeys
    void shutdown();

    // Handle WM_HOTKEY message, returns true if handled
    bool handleMessage(WPARAM wParam);

    // Callbacks
    std::function<void()> on_increase;
    std::function<void()> on_decrease;
    std::function<void()> on_reset;

    // Hotkey IDs
    static constexpr int ID_INCREASE = 1;
    static constexpr int ID_DECREASE = 2;
    static constexpr int ID_RESET = 3;

    // Default key bindings
    static constexpr UINT MOD_KEYS = MOD_CONTROL | MOD_ALT;
    static constexpr UINT VK_INCREASE = VK_UP;      // Ctrl+Alt+Up
    static constexpr UINT VK_DECREASE = VK_DOWN;    // Ctrl+Alt+Down
    static constexpr UINT VK_RESET_KEY = 'R';       // Ctrl+Alt+R

private:
    HWND hwnd_ = nullptr;
    bool registered_ = false;
};

} // namespace lumos::platform
