// Lumos - Main UI window
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#pragma once

namespace lumos {
class App;
}

namespace lumos::ui {

class MainWindow {
public:
    MainWindow() = default;

    // Render the window (call each frame)
    void render(App& app);

    // Open tabs
    void openHelp() { show_help_tab_ = true; }
    void openAbout() { show_about_tab_ = true; }

    // Open windows
    void openTestPattern() { show_test_pattern_window_ = true; }

private:
    void renderMenuBar();
    void renderGammaTab(App& app);
    void renderHelpTab();
    void renderAboutTab();
    void renderTestPatternWindow();

    float gamma_slider_ = 1.0f;
    int transfer_function_index_ = 0;  // 0 = Power (default)
    bool first_frame_ = true;

    // Tab visibility
    bool show_help_tab_ = false;
    bool show_about_tab_ = false;

    // Separate window visibility
    bool show_test_pattern_window_ = false;
};

} // namespace lumos::ui
