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
    // Use default bindings
    return initialize(hwnd,
                      { MOD_CONTROL | MOD_ALT, VK_UP },
                      { MOD_CONTROL | MOD_ALT, VK_DOWN },
                      { MOD_CONTROL | MOD_ALT, 'R' },
                      { MOD_CONTROL | MOD_ALT, 'G' });
}

bool Hotkeys::initialize(HWND hwnd,
                         const HotkeyBinding& increase,
                         const HotkeyBinding& decrease,
                         const HotkeyBinding& reset,
                         const HotkeyBinding& toggle)
{
    if (registered_) return true;

    hwnd_ = hwnd;

    // Store bindings
    binding_increase_ = increase;
    binding_decrease_ = decrease;
    binding_reset_ = reset;
    binding_toggle_ = toggle;

    // Register global hotkeys
    bool success = true;

    if (!registerSingle(ID_INCREASE, binding_increase_)) {
        success = false;
    }

    if (!registerSingle(ID_DECREASE, binding_decrease_)) {
        success = false;
    }

    if (!registerSingle(ID_RESET, binding_reset_)) {
        success = false;
    }

    if (!registerSingle(ID_TOGGLE, binding_toggle_)) {
        success = false;
    }

    registered_ = true;
    return success;
}

bool Hotkeys::registerSingle(int id, const HotkeyBinding& binding)
{
    return RegisterHotKey(hwnd_, id, binding.modifiers | MOD_NOREPEAT, binding.key) != 0;
}

void Hotkeys::shutdown()
{
    if (!registered_ || !hwnd_) return;

    UnregisterHotKey(hwnd_, ID_INCREASE);
    UnregisterHotKey(hwnd_, ID_DECREASE);
    UnregisterHotKey(hwnd_, ID_RESET);
    UnregisterHotKey(hwnd_, ID_TOGGLE);

    registered_ = false;
}

bool Hotkeys::reregister(const HotkeyBinding& increase,
                         const HotkeyBinding& decrease,
                         const HotkeyBinding& reset,
                         const HotkeyBinding& toggle,
                         RegistrationResult* result)
{
    if (!hwnd_) return false;

    // Unregister existing hotkeys
    UnregisterHotKey(hwnd_, ID_INCREASE);
    UnregisterHotKey(hwnd_, ID_DECREASE);
    UnregisterHotKey(hwnd_, ID_RESET);
    UnregisterHotKey(hwnd_, ID_TOGGLE);

    RegistrationResult res;

    // Register new bindings
    res.increase_ok = registerSingle(ID_INCREASE, increase);
    res.decrease_ok = registerSingle(ID_DECREASE, decrease);
    res.reset_ok = registerSingle(ID_RESET, reset);
    res.toggle_ok = registerSingle(ID_TOGGLE, toggle);

    if (!res.increase_ok || !res.decrease_ok || !res.reset_ok || !res.toggle_ok) {
        res.last_error = GetLastError();
    }

    // Store new bindings
    binding_increase_ = increase;
    binding_decrease_ = decrease;
    binding_reset_ = reset;
    binding_toggle_ = toggle;

    if (result) *result = res;

    return res.increase_ok && res.decrease_ok && res.reset_ok && res.toggle_ok;
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
    case ID_TOGGLE:
        if (on_toggle) on_toggle();
        return true;
    }
    return false;
}

} // namespace lumos::platform
