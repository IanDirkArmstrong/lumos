// Lumos - Global hotkeys module
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <functional>
#include "../../config.h"

namespace lumos::platform {

class Hotkeys {
public:
    Hotkeys() = default;
    ~Hotkeys();

    // Initialize hotkeys with default bindings (call after window creation)
    bool initialize(HWND hwnd);

    // Initialize hotkeys with custom bindings
    bool initialize(HWND hwnd,
                    const HotkeyBinding& increase,
                    const HotkeyBinding& decrease,
                    const HotkeyBinding& reset,
                    const HotkeyBinding& toggle);

    // Cleanup hotkeys
    void shutdown();

    // Handle WM_HOTKEY message, returns true if handled
    bool handleMessage(WPARAM wParam);

    // Registration result for error feedback
    struct RegistrationResult {
        bool increase_ok = false;
        bool decrease_ok = false;
        bool reset_ok = false;
        bool toggle_ok = false;
        DWORD last_error = 0;
    };

    // Re-register hotkeys with new bindings at runtime
    bool reregister(const HotkeyBinding& increase,
                    const HotkeyBinding& decrease,
                    const HotkeyBinding& reset,
                    const HotkeyBinding& toggle,
                    RegistrationResult* result = nullptr);

    // Get current bindings
    HotkeyBinding getIncreaseBinding() const { return binding_increase_; }
    HotkeyBinding getDecreaseBinding() const { return binding_decrease_; }
    HotkeyBinding getResetBinding() const { return binding_reset_; }
    HotkeyBinding getToggleBinding() const { return binding_toggle_; }

    // Callbacks
    std::function<void()> on_increase;
    std::function<void()> on_decrease;
    std::function<void()> on_reset;
    std::function<void()> on_toggle;

    // Hotkey IDs
    static constexpr int ID_INCREASE = 1;
    static constexpr int ID_DECREASE = 2;
    static constexpr int ID_RESET = 3;
    static constexpr int ID_TOGGLE = 4;

    // Default key bindings
    static constexpr UINT MOD_KEYS = MOD_CONTROL | MOD_ALT;
    static constexpr UINT VK_INCREASE = VK_UP;      // Ctrl+Alt+Up
    static constexpr UINT VK_DECREASE = VK_DOWN;    // Ctrl+Alt+Down
    static constexpr UINT VK_RESET_KEY = 'R';       // Ctrl+Alt+R
    static constexpr UINT VK_TOGGLE_KEY = 'G';      // Ctrl+Alt+G

private:
    bool registerSingle(int id, const HotkeyBinding& binding);

    HWND hwnd_ = nullptr;
    bool registered_ = false;

    // Current bindings
    HotkeyBinding binding_increase_ = { MOD_CONTROL | MOD_ALT, VK_UP };
    HotkeyBinding binding_decrease_ = { MOD_CONTROL | MOD_ALT, VK_DOWN };
    HotkeyBinding binding_reset_ = { MOD_CONTROL | MOD_ALT, 'R' };
    HotkeyBinding binding_toggle_ = { MOD_CONTROL | MOD_ALT, 'G' };
};

} // namespace lumos::platform
