// Lumos - Application orchestration
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#pragma once

#include "config.h"
#include "platform/win32/gamma.h"
#include "platform/win32/tray.h"
#include "platform/win32/hotkeys.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <functional>

namespace lumos {

class App {
public:
    App() = default;

    // Initialize application (call after window creation)
    bool initialize(HWND hwnd, UINT tray_msg);

    // Shutdown application (call before exit)
    void shutdown();

    // Set gamma value (applies immediately to all monitors)
    void setGamma(double value);

    // Set transfer function (applies immediately with current gamma)
    void setTransferFunction(platform::TransferFunction func);

    // Adjust gamma by delta
    void adjustGamma(double delta);

    // Reset to original gamma
    void resetGamma();

    // Get current gamma value
    double getGamma() const { return current_gamma_; }

    // Get current transfer function
    platform::TransferFunction getTransferFunction() const { return transfer_function_; }

    // Window visibility
    void showWindow();
    void hideWindow();
    bool isWindowVisible() const { return window_visible_; }

    // Show dialogs
    void showHelp();
    void showAbout();

    // Request application exit
    void requestExit();
    bool shouldExit() const { return should_exit_; }

    // Callbacks for showing dialogs (set from main.cpp)
    std::function<void()> on_show_help;
    std::function<void()> on_show_about;

    // Handle tray messages
    bool handleTrayMessage(WPARAM wParam, LPARAM lParam);

    // Handle hotkey messages
    bool handleHotkeyMessage(WPARAM wParam);

    // Get status text for UI
    const char* getStatusText() const { return status_text_; }

    // Get monitor count
    size_t getMonitorCount() const { return gamma_.getMonitorCount(); }

    // Get reference to gamma module (for crash handler)
    platform::Gamma& getGammaRef() { return gamma_; }

private:
    Config config_;
    platform::Gamma gamma_;
    platform::Tray tray_;
    platform::Hotkeys hotkeys_;

    HWND hwnd_ = nullptr;
    double current_gamma_ = 1.0;
    platform::TransferFunction transfer_function_ = platform::TransferFunction::Power;
    bool window_visible_ = true;
    bool should_exit_ = false;
    char status_text_[64] = "Ready";

    static constexpr double GAMMA_STEP = 0.1;
};

} // namespace lumos
