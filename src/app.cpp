// Lumos - Application orchestration
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#include "app.h"
#include <cstdio>
#include <algorithm>

namespace lumos {

namespace {
// Helper function to convert config string to ToneCurve enum
platform::ToneCurve stringToToneCurve(const std::string& str) {
    if (str == "Linear") return platform::ToneCurve::Linear;
    if (str == "ShadowLift") return platform::ToneCurve::ShadowLift;
    if (str == "SoftContrast") return platform::ToneCurve::SoftContrast;
    if (str == "Cinema") return platform::ToneCurve::Cinema;
    if (str == "Custom") return platform::ToneCurve::Custom;
    // Legacy config migration
    if (str == "sRGB") return platform::ToneCurve::ShadowLift;
    if (str == "Rec709" || str == "Rec2020") return platform::ToneCurve::SoftContrast;
    if (str == "DCIP3") return platform::ToneCurve::Cinema;
    return platform::ToneCurve::Power;  // default
}

// Helper function to convert ToneCurve enum to config string
std::string toneCurveToString(platform::ToneCurve curve) {
    switch (curve) {
        case platform::ToneCurve::Linear: return "Linear";
        case platform::ToneCurve::ShadowLift: return "ShadowLift";
        case platform::ToneCurve::SoftContrast: return "SoftContrast";
        case platform::ToneCurve::Cinema: return "Cinema";
        case platform::ToneCurve::Custom: return "Custom";
        case platform::ToneCurve::Power:
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
    tone_curve_ = stringToToneCurve(config_.transfer_function);
    custom_curve_points_ = config_.custom_curve_points;

    // Ensure custom curve has valid default if empty
    if (custom_curve_points_.empty()) {
        custom_curve_points_ = {{0.0, 0.0}, {1.0, 1.0}};  // Linear default
    }

    // Initialize gamma (captures original ramps for all monitors)
    if (!gamma_.initialize()) {
        std::snprintf(status_text_, sizeof(status_text_), "Warning: Could not initialize gamma");
    }

    // Apply saved tone curve to all monitors
    if (current_gamma_ != 1.0 || tone_curve_ != platform::ToneCurve::Power) {
        if (tone_curve_ == platform::ToneCurve::Custom) {
            gamma_.applyAll(tone_curve_, current_gamma_, &custom_curve_points_);
        } else {
            gamma_.applyAll(tone_curve_, current_gamma_);
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
    tray_.on_close_to_tray = [this]() { hideWindow(); };
    tray_.on_exit = [this]() { requestExit(); };

    // Initialize hotkeys with config bindings
    if (!hotkeys_.initialize(hwnd_,
                             config_.hotkey_increase,
                             config_.hotkey_decrease,
                             config_.hotkey_reset,
                             config_.hotkey_toggle)) {
        std::snprintf(status_text_, sizeof(status_text_), "Warning: Some hotkeys failed to register");
    }

    // Set up hotkey callbacks
    hotkeys_.on_increase = [this]() { adjustGamma(GAMMA_STEP); };
    hotkeys_.on_decrease = [this]() { adjustGamma(-GAMMA_STEP); };
    hotkeys_.on_reset = [this]() { resetGamma(); };
    hotkeys_.on_toggle = [this]() { toggleGamma(); };

    // Start screen histogram capture
    histogram_.start();

    size_t count = gamma_.getMonitorCount();
    std::snprintf(status_text_, sizeof(status_text_), "Applied to %zu display%s",
                  count, count == 1 ? "" : "s");
    return true;
}

void App::shutdown()
{
    // Stop screen histogram capture
    histogram_.stop();

    // Shutdown hotkeys
    hotkeys_.shutdown();

    // Save config (hotkey bindings are already in config_ after any setHotkeys calls)
    config_.last_gamma = current_gamma_;
    config_.transfer_function = toneCurveToString(tone_curve_);
    config_.custom_curve_points = custom_curve_points_;
    // Hotkey bindings are updated by setHotkeys() and stored in config_
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
    if (tone_curve_ == platform::ToneCurve::Custom) {
        success = gamma_.applyAll(tone_curve_, value, &custom_curve_points_);
    } else {
        success = gamma_.applyAll(tone_curve_, value);
    }

    if (success) {
        size_t count = gamma_.getMonitorCount();
        std::snprintf(status_text_, sizeof(status_text_), "Applied to %zu display%s",
                      count, count == 1 ? "" : "s");
    } else {
        std::snprintf(status_text_, sizeof(status_text_), "Failed to apply gamma");
    }
}

void App::setToneCurve(platform::ToneCurve curve)
{
    tone_curve_ = curve;
    // Reapply current strength with new tone curve
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
        std::snprintf(status_text_, sizeof(status_text_), "Restored captured defaults");
    } else {
        if (tone_curve_ == platform::ToneCurve::Custom) {
            gamma_.applyAll(tone_curve_, 1.0, &custom_curve_points_);
        } else {
            gamma_.applyAll(tone_curve_, 1.0);
        }
        std::snprintf(status_text_, sizeof(status_text_), "Reset to linear");
    }
}

void App::setCustomCurvePoints(const std::vector<platform::CurvePoint>& points)
{
    custom_curve_points_ = points;

    // If we're currently in Custom mode, reapply immediately
    if (tone_curve_ == platform::ToneCurve::Custom) {
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

bool App::setHotkeys(const HotkeyBinding& increase,
                     const HotkeyBinding& decrease,
                     const HotkeyBinding& reset,
                     const HotkeyBinding& toggle)
{
    platform::Hotkeys::RegistrationResult result;
    bool success = hotkeys_.reregister(increase, decrease, reset, toggle, &result);

    if (success) {
        // Update config (will be persisted on shutdown)
        config_.hotkey_increase = increase;
        config_.hotkey_decrease = decrease;
        config_.hotkey_reset = reset;
        config_.hotkey_toggle = toggle;
        hotkey_error_[0] = '\0';
        std::snprintf(status_text_, sizeof(status_text_), "Hotkeys updated");
    } else {
        // Build error message indicating which hotkeys failed
        std::string err;
        if (!result.increase_ok) err += "Increase ";
        if (!result.decrease_ok) err += "Decrease ";
        if (!result.reset_ok) err += "Reset ";
        if (!result.toggle_ok) err += "Toggle ";
        err += "hotkey(s) failed - may be in use by another app";
        std::snprintf(hotkey_error_, sizeof(hotkey_error_), "%s", err.c_str());
    }

    return success;
}

void App::toggleGamma()
{
    if (gamma_enabled_) {
        // Disable: store current gamma and restore original
        gamma_before_disable_ = current_gamma_;
        gamma_.restoreAll();
        gamma_enabled_ = false;
        std::snprintf(status_text_, sizeof(status_text_), "Gamma OFF");
    } else {
        // Enable: reapply the stored gamma value
        gamma_enabled_ = true;
        if (tone_curve_ == platform::ToneCurve::Custom) {
            gamma_.applyAll(tone_curve_, gamma_before_disable_, &custom_curve_points_);
        } else {
            gamma_.applyAll(tone_curve_, gamma_before_disable_);
        }
        current_gamma_ = gamma_before_disable_;
        std::snprintf(status_text_, sizeof(status_text_), "Gamma ON (%.1f)", current_gamma_);
    }
}

} // namespace lumos
