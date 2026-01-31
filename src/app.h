// Lumos - Application orchestration
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#pragma once

#include "config.h"
#include "platform/win32/gamma.h"
#include "platform/win32/tray.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

namespace lumos {

class App {
public:
    App() = default;

    // Initialize application (call after window creation)
    bool initialize(HWND hwnd, UINT tray_msg);

    // Shutdown application (call before exit)
    void shutdown();

    // Set gamma value (applies immediately)
    void setGamma(double value);

    // Reset to original gamma
    void resetGamma();

    // Get current gamma value
    double getGamma() const { return current_gamma_; }

    // Window visibility
    void showWindow();
    void hideWindow();
    bool isWindowVisible() const { return window_visible_; }

    // Request application exit
    void requestExit();
    bool shouldExit() const { return should_exit_; }

    // Handle tray messages
    bool handleTrayMessage(WPARAM wParam, LPARAM lParam);

    // Get status text for UI
    const char* getStatusText() const { return status_text_; }

    // Get reference to gamma module (for crash handler)
    platform::Gamma& getGammaRef() { return gamma_; }

private:
    Config config_;
    platform::Gamma gamma_;
    platform::Tray tray_;

    HWND hwnd_ = nullptr;
    double current_gamma_ = 1.0;
    bool window_visible_ = true;
    bool should_exit_ = false;
    char status_text_[64] = "Ready";
};

} // namespace lumos
