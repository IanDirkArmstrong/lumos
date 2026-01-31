// Lumos - Global hotkeys module
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#include "hotkeys.h"

namespace lumos::platform {

Hotkeys::~Hotkeys()
{
    shutdown();
}

bool Hotkeys::initialize(HWND hwnd)
{
    if (registered_) return true;

    hwnd_ = hwnd;

    // Register global hotkeys
    bool success = true;

    if (!RegisterHotKey(hwnd_, ID_INCREASE, MOD_KEYS | MOD_NOREPEAT, VK_INCREASE)) {
        success = false;
    }

    if (!RegisterHotKey(hwnd_, ID_DECREASE, MOD_KEYS | MOD_NOREPEAT, VK_DECREASE)) {
        success = false;
    }

    if (!RegisterHotKey(hwnd_, ID_RESET, MOD_KEYS | MOD_NOREPEAT, VK_RESET_KEY)) {
        success = false;
    }

    registered_ = true;
    return success;
}

void Hotkeys::shutdown()
{
    if (!registered_ || !hwnd_) return;

    UnregisterHotKey(hwnd_, ID_INCREASE);
    UnregisterHotKey(hwnd_, ID_DECREASE);
    UnregisterHotKey(hwnd_, ID_RESET);

    registered_ = false;
}

bool Hotkeys::handleMessage(WPARAM wParam)
{
    switch (wParam) {
    case ID_INCREASE:
        if (on_increase) on_increase();
        return true;
    case ID_DECREASE:
        if (on_decrease) on_decrease();
        return true;
    case ID_RESET:
        if (on_reset) on_reset();
        return true;
    }
    return false;
}

} // namespace lumos::platform
