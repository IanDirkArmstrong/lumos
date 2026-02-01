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
    void openTestPattern() { show_test_pattern_tab_ = true; }

private:
    void renderMenuBar();
    void renderGammaTab(App& app);
    void renderHelpTab();
    void renderAboutTab();
    void renderTestPatternTab();

    float gamma_slider_ = 1.0f;
    bool first_frame_ = true;

    // Tab visibility
    bool show_help_tab_ = false;
    bool show_about_tab_ = false;
    bool show_test_pattern_tab_ = false;
};

} // namespace lumos::ui
