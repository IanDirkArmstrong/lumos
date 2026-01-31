// Lumos - Application orchestration
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#include "app.h"
#include <cstdio>

namespace lumos {

bool App::initialize(HWND hwnd, UINT tray_msg)
{
    hwnd_ = hwnd;

    // Load config
    config_.load();
    current_gamma_ = config_.last_gamma;

    // Capture original gamma for restoration
    if (!gamma_.captureOriginal()) {
        std::snprintf(status_text_, sizeof(status_text_), "Warning: Could not capture original gamma");
    }

    // Apply saved gamma
    if (current_gamma_ != 1.0) {
        gamma_.apply(current_gamma_);
    }

    // Create tray icon
    if (!tray_.create(hwnd_, tray_msg)) {
        std::snprintf(status_text_, sizeof(status_text_), "Warning: Could not create tray icon");
    }

    // Set up tray callbacks
    tray_.on_open = [this]() { showWindow(); };
    tray_.on_reset = [this]() { resetGamma(); };
    tray_.on_exit = [this]() { requestExit(); };

    std::snprintf(status_text_, sizeof(status_text_), "Applied to primary display");
    return true;
}

void App::shutdown()
{
    // Save config
    config_.last_gamma = current_gamma_;
    config_.save();

    // Restore original gamma
    gamma_.restoreOriginal();

    // Remove tray icon
    tray_.destroy();
}

void App::setGamma(double value)
{
    if (value < 0.1) value = 0.1;
    if (value > 9.0) value = 9.0;

    current_gamma_ = value;

    if (gamma_.apply(value)) {
        std::snprintf(status_text_, sizeof(status_text_), "Applied to primary display");
    } else {
        std::snprintf(status_text_, sizeof(status_text_), "Failed to apply gamma");
    }
}

void App::resetGamma()
{
    current_gamma_ = 1.0;

    if (gamma_.restoreOriginal()) {
        std::snprintf(status_text_, sizeof(status_text_), "Reset to original");
    } else {
        // Fall back to setting gamma to 1.0
        gamma_.apply(1.0);
        std::snprintf(status_text_, sizeof(status_text_), "Reset to default (1.0)");
    }
}

void App::showWindow()
{
    if (hwnd_) {
        ShowWindow(hwnd_, SW_SHOW);
        SetForegroundWindow(hwnd_);
    }
    window_visible_ = true;
}

void App::hideWindow()
{
    if (hwnd_) {
        ShowWindow(hwnd_, SW_HIDE);
    }
    window_visible_ = false;
}

void App::requestExit()
{
    should_exit_ = true;
    if (hwnd_) {
        PostMessageW(hwnd_, WM_CLOSE, 0, 0);
    }
}

bool App::handleTrayMessage(WPARAM wParam, LPARAM lParam)
{
    return tray_.handleMessage(wParam, lParam);
}

} // namespace lumos
