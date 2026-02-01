// Lumos - Application orchestration
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#include "app.h"
#include <cstdio>
#include <algorithm>

namespace lumos {

bool App::initialize(HWND hwnd, UINT tray_msg)
{
    hwnd_ = hwnd;

    // Load config
    config_.load();
    current_gamma_ = config_.last_gamma;

    // Initialize gamma (captures original ramps for all monitors)
    if (!gamma_.initialize()) {
        std::snprintf(status_text_, sizeof(status_text_), "Warning: Could not initialize gamma");
    }

    // Apply saved gamma to all monitors
    if (current_gamma_ != 1.0) {
        gamma_.applyAll(transfer_function_, current_gamma_);
    }

    // Create tray icon
    if (!tray_.create(hwnd_, tray_msg)) {
        std::snprintf(status_text_, sizeof(status_text_), "Warning: Could not create tray icon");
    }

    // Set up tray callbacks
    tray_.on_open = [this]() { showWindow(); };
    tray_.on_reset = [this]() { resetGamma(); };
    tray_.on_help = [this]() { showHelp(); };
    tray_.on_about = [this]() { showAbout(); };
    tray_.on_exit = [this]() { requestExit(); };

    // Initialize hotkeys
    if (!hotkeys_.initialize(hwnd_)) {
        std::snprintf(status_text_, sizeof(status_text_), "Warning: Some hotkeys failed to register");
    }

    // Set up hotkey callbacks
    hotkeys_.on_increase = [this]() { adjustGamma(GAMMA_STEP); };
    hotkeys_.on_decrease = [this]() { adjustGamma(-GAMMA_STEP); };
    hotkeys_.on_reset = [this]() { resetGamma(); };

    size_t count = gamma_.getMonitorCount();
    std::snprintf(status_text_, sizeof(status_text_), "Applied to %zu display%s",
                  count, count == 1 ? "" : "s");
    return true;
}

void App::shutdown()
{
    // Shutdown hotkeys
    hotkeys_.shutdown();

    // Save config
    config_.last_gamma = current_gamma_;
    config_.save();

    // Restore original gamma on all monitors
    gamma_.restoreAll();

    // Remove tray icon
    tray_.destroy();
}

void App::setGamma(double value)
{
    value = std::clamp(value, 0.1, 9.0);
    current_gamma_ = value;

    if (gamma_.applyAll(transfer_function_, value)) {
        size_t count = gamma_.getMonitorCount();
        std::snprintf(status_text_, sizeof(status_text_), "Applied to %zu display%s",
                      count, count == 1 ? "" : "s");
    } else {
        std::snprintf(status_text_, sizeof(status_text_), "Failed to apply gamma");
    }
}

void App::setTransferFunction(platform::TransferFunction func)
{
    transfer_function_ = func;
    // Reapply current gamma with new transfer function
    setGamma(current_gamma_);
}

void App::adjustGamma(double delta)
{
    setGamma(current_gamma_ + delta);
}

void App::resetGamma()
{
    current_gamma_ = 1.0;

    if (gamma_.restoreAll()) {
        std::snprintf(status_text_, sizeof(status_text_), "Reset to original");
    } else {
        gamma_.applyAll(transfer_function_, 1.0);
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

void App::showHelp()
{
    showWindow(); // Ensure window is visible
    if (on_show_help) {
        on_show_help();
    }
}

void App::showAbout()
{
    showWindow(); // Ensure window is visible
    if (on_show_about) {
        on_show_about();
    }
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

bool App::handleHotkeyMessage(WPARAM wParam)
{
    return hotkeys_.handleMessage(wParam);
}

} // namespace lumos
