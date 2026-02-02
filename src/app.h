// Lumos - Application orchestration
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#pragma once

#include "config.h"
#include "platform/win32/gamma.h"
#include "platform/win32/screen_histogram.h"
#include "platform/win32/tray.h"
#include "platform/win32/hotkeys.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
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

    // Set tone curve preset (applies immediately)
    void setToneCurve(platform::ToneCurve curve);

    // Adjust gamma by delta
    void adjustGamma(double delta);

    // Reset to original gamma
    void resetGamma();

    // Get current gamma value
    double getGamma() const { return current_gamma_; }

    // Get current tone curve preset
    platform::ToneCurve getToneCurve() const { return tone_curve_; }

    // Custom curve management
    const std::vector<platform::CurvePoint>& getCustomCurvePoints() const { return custom_curve_points_; }
    void setCustomCurvePoints(const std::vector<platform::CurvePoint>& points);

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

    // Hotkey configuration
    HotkeyBinding getHotkeyIncrease() const { return config_.hotkey_increase; }
    HotkeyBinding getHotkeyDecrease() const { return config_.hotkey_decrease; }
    HotkeyBinding getHotkeyReset() const { return config_.hotkey_reset; }
    HotkeyBinding getHotkeyToggle() const { return config_.hotkey_toggle; }

    // Apply new hotkey bindings (returns success, sets error message)
    bool setHotkeys(const HotkeyBinding& increase,
                    const HotkeyBinding& decrease,
                    const HotkeyBinding& reset,
                    const HotkeyBinding& toggle);

    // Get last hotkey error message (empty if no error)
    const char* getHotkeyError() const { return hotkey_error_; }

    // Toggle gamma on/off
    void toggleGamma();
    bool isGammaEnabled() const { return gamma_enabled_; }

    // Window behavior settings
    bool getMinimizeToTrayOnClose() const { return config_.minimize_to_tray_on_close; }
    void setMinimizeToTrayOnClose(bool value) { config_.minimize_to_tray_on_close = value; }

    // Get monitor count
    size_t getMonitorCount() const { return gamma_.getMonitorCount(); }

    // Get reference to gamma module (for crash handler)
    platform::Gamma& getGammaRef() { return gamma_; }

    // Screen histogram access
    platform::ScreenHistogram getScreenHistogram() const { return histogram_.getHistogram(); }
    void setHistogramEnabled(bool enabled) { histogram_.setEnabled(enabled); }
    bool isHistogramEnabled() const { return histogram_.isEnabled(); }

private:
    Config config_;
    platform::Gamma gamma_;
    platform::ScreenHistogramCapture histogram_;
    platform::Tray tray_;
    platform::Hotkeys hotkeys_;

    HWND hwnd_ = nullptr;
    double current_gamma_ = 1.0;
    platform::ToneCurve tone_curve_ = platform::ToneCurve::Power;
    std::vector<platform::CurvePoint> custom_curve_points_;
    bool window_visible_ = true;
    bool should_exit_ = false;
    char status_text_[64] = "Ready";
    char hotkey_error_[128] = "";

    // Toggle state
    bool gamma_enabled_ = true;
    double gamma_before_disable_ = 1.0;

    static constexpr double GAMMA_STEP = 0.1;
};

} // namespace lumos
