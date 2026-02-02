// Lumos - Application orchestration
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#include "app.h"
#include <cstdio>
#include <algorithm>

namespace lumos {

namespace {
// Helper function to convert string to TransferFunction enum
platform::TransferFunction stringToTransferFunction(const std::string& str) {
    if (str == "sRGB") return platform::TransferFunction::sRGB;
    if (str == "Rec709") return platform::TransferFunction::Rec709;
    if (str == "Rec2020") return platform::TransferFunction::Rec2020;
    if (str == "DCIP3") return platform::TransferFunction::DCIP3;
    if (str == "Custom") return platform::TransferFunction::Custom;
    return platform::TransferFunction::Power;  // default
}

// Helper function to convert TransferFunction enum to string
std::string transferFunctionToString(platform::TransferFunction func) {
    switch (func) {
        case platform::TransferFunction::sRGB: return "sRGB";
        case platform::TransferFunction::Rec709: return "Rec709";
        case platform::TransferFunction::Rec2020: return "Rec2020";
        case platform::TransferFunction::DCIP3: return "DCIP3";
        case platform::TransferFunction::Custom: return "Custom";
        case platform::TransferFunction::Power:
        default: return "Power";
    }
}
} // anonymous namespace

bool App::initialize(HWND hwnd, UINT tray_msg)
{
    hwnd_ = hwnd;

    // Load config
    config_.load();
    current_gamma_ = config_.last_gamma;
    transfer_function_ = stringToTransferFunction(config_.transfer_function);
    custom_curve_points_ = config_.custom_curve_points;

    // Ensure custom curve has valid default if empty
    if (custom_curve_points_.empty()) {
        custom_curve_points_ = {{0.0, 0.0}, {1.0, 1.0}};  // Linear default
    }

    // Initialize gamma (captures original ramps for all monitors)
    if (!gamma_.initialize()) {
        std::snprintf(status_text_, sizeof(status_text_), "Warning: Could not initialize gamma");
    }

    // Apply saved gamma to all monitors
    if (current_gamma_ != 1.0 || transfer_function_ != platform::TransferFunction::Power) {
        if (transfer_function_ == platform::TransferFunction::Custom) {
            gamma_.applyAll(transfer_function_, current_gamma_, &custom_curve_points_);
        } else {
            gamma_.applyAll(transfer_function_, current_gamma_);
        }
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
    config_.transfer_function = transferFunctionToString(transfer_function_);
    config_.custom_curve_points = custom_curve_points_;
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

    bool success;
    if (transfer_function_ == platform::TransferFunction::Custom) {
        success = gamma_.applyAll(transfer_function_, value, &custom_curve_points_);
    } else {
        success = gamma_.applyAll(transfer_function_, value);
    }

    if (success) {
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
        if (transfer_function_ == platform::TransferFunction::Custom) {
            gamma_.applyAll(transfer_function_, 1.0, &custom_curve_points_);
        } else {
            gamma_.applyAll(transfer_function_, 1.0);
        }
        std::snprintf(status_text_, sizeof(status_text_), "Reset to default (1.0)");
    }
}

void App::setCustomCurvePoints(const std::vector<platform::CurvePoint>& points)
{
    custom_curve_points_ = points;

    // If we're currently in Custom mode, reapply immediately
    if (transfer_function_ == platform::TransferFunction::Custom) {
        setGamma(current_gamma_);
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
