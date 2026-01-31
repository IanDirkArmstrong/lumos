// Lumos - About dialog
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#pragma once

namespace lumos::ui {

class AboutDialog {
public:
    AboutDialog() = default;

    // Show the dialog (call each frame when visible)
    void render(bool* p_open);

    // Open the dialog
    void open() { visible_ = true; }

    // Check if visible
    bool isVisible() const { return visible_; }

private:
    bool visible_ = false;
};

} // namespace lumos::ui
