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

private:
    float gamma_slider_ = 1.0f;
    bool first_frame_ = true;
};

} // namespace lumos::ui
