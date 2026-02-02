// Lumos - Main UI window
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#pragma once

#include <array>
#include <vector>
#include "../platform/win32/gamma.h"
#include "../config.h"

namespace lumos {
class App;
}

namespace lumos::ui {

class MainWindow {
public:
    MainWindow() = default;

    // Render the window (call each frame)
    void render(App& app);

    // Open tabs (and focus them)
    void openHelp() { show_help_tab_ = true; focus_help_tab_ = true; }
    void openAbout() { show_about_tab_ = true; focus_about_tab_ = true; }
    void openSettings() { show_settings_tab_ = true; focus_settings_tab_ = true; }

    // Open windows
    void openTestPattern() { show_test_pattern_window_ = true; }

private:
    void renderMenuBar();
    void renderStatusBar(App& app);
    void renderGammaTab(App& app);
    void renderHelpTab();
    void renderAboutTab();
    void renderSettingsTab(App& app);
    void renderTestPatternWindow();

    float gamma_slider_ = 1.0f;
    int transfer_function_index_ = 0;  // 0 = Power (default)
    bool first_frame_ = true;

    // Custom curve editor state
    std::vector<lumos::platform::CurvePoint> ui_curve_points_;
    std::vector<lumos::platform::CurvePoint> reference_curve_points_; // Baseline for snapping/overlay
    int selected_point_index_ = -1;  // -1 = none selected
    bool dragging_point_ = false;

    // Histogram display
    bool show_histogram_ = true;
    std::array<float, 256> histogram_xs_{};   // X values (0-1)
    std::array<float, 256> histogram_ys_{};   // Y values (normalized)
    std::array<float, 256> histogram_ys_post_{}; // After tone curve
    std::array<float, 256> histogram_diff_{};    // Post - pre

    // Tab visibility
    bool show_help_tab_ = false;
    bool show_about_tab_ = false;
    bool show_settings_tab_ = false;

    // Tab focus (set when opening to make it the active tab)
    bool focus_help_tab_ = false;
    bool focus_about_tab_ = false;
    bool focus_settings_tab_ = false;

    // Settings tab state
    HotkeyBinding edit_hotkey_increase_;
    HotkeyBinding edit_hotkey_decrease_;
    HotkeyBinding edit_hotkey_reset_;
    HotkeyBinding edit_hotkey_toggle_;
    bool hotkey_settings_initialized_ = false;
    bool hotkey_settings_dirty_ = false;

    // Separate window visibility
    bool show_test_pattern_window_ = false;
};

} // namespace lumos::ui
