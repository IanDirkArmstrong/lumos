// Lumos - Main UI window
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#include "main_window.h"
#include "../app.h"
#include "imgui.h"
#include <cmath>
#include <cstdio>

namespace lumos::ui {

void MainWindow::render(App& app)
{
    // Always sync slider with app state (in case it was changed by hotkeys or tray menu)
    // Only update if not actively being dragged to avoid fighting with user input
    float current_gamma = static_cast<float>(app.getGamma());
    if (!ImGui::IsAnyItemActive() || first_frame_) {
        gamma_slider_ = current_gamma;
    }
    first_frame_ = false;

    // Full window ImGui panel
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_MenuBar;

    ImGui::Begin("##MainWindow", nullptr, flags);

    // Render menu bar
    renderMenuBar();

    // Render tab bar
    if (ImGui::BeginTabBar("MainTabBar", ImGuiTabBarFlags_None)) {
        // Gamma control tab (always visible, not closable)
        if (ImGui::BeginTabItem("\xE2\x9A\x99 Gamma")) {  // ⚙ Gamma
            renderGammaTab(app);
            ImGui::EndTabItem();
        }

        // Help tab (closable)
        if (show_help_tab_) {
            if (ImGui::BeginTabItem("\xE2\x9D\x93 Help", &show_help_tab_)) {  // ❓ Help
                renderHelpTab();
                ImGui::EndTabItem();
            }
        }

        // About tab (closable)
        if (show_about_tab_) {
            if (ImGui::BeginTabItem("\xE2\x84\xB9 About", &show_about_tab_)) {  // ℹ About
                renderAboutTab();
                ImGui::EndTabItem();
            }
        }

        // Test Pattern tab (closable)
        if (show_test_pattern_tab_) {
            if (ImGui::BeginTabItem("\xE2\x96\xA6 Pattern", &show_test_pattern_tab_)) {  // ▦ Pattern
                renderTestPatternTab();
                ImGui::EndTabItem();
            }
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void MainWindow::renderMenuBar()
{
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit")) {
                // Request exit via WM_CLOSE
                PostMessageW(GetActiveWindow(), WM_CLOSE, 0, 0);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Help", "F1")) {
                show_help_tab_ = true;
            }
            if (ImGui::MenuItem("About")) {
                show_about_tab_ = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Test Pattern")) {
                show_test_pattern_tab_ = true;
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void MainWindow::renderGammaTab(App& app)
{
    ImGui::Spacing();

    // Gamma slider
    ImGui::TextUnformatted("Gamma");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##Gamma", &gamma_slider_, 0.1f, 9.0f, "%.2f")) {
        app.setGamma(static_cast<double>(gamma_slider_));
    }

    // Draw tick marks at common gamma values
    {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 slider_min = ImGui::GetItemRectMin();
        ImVec2 slider_max = ImGui::GetItemRectMax();
        float slider_width = slider_max.x - slider_min.x;

        // Common gamma values to mark
        const float tick_values[] = { 1.0f, 1.8f, 2.2f, 2.5f };
        const float gamma_min = 0.1f;
        const float gamma_max = 9.0f;

        for (float tick_val : tick_values) {
            // Calculate position (slider uses linear mapping)
            float t = (tick_val - gamma_min) / (gamma_max - gamma_min);
            float x = slider_min.x + t * slider_width;
            float y_top = slider_max.y;
            float y_bottom = slider_max.y + 6.0f;

            // Draw tick line
            draw_list->AddLine(
                ImVec2(x, y_top),
                ImVec2(x, y_bottom),
                IM_COL32(150, 150, 150, 255), 1.0f);

            // Draw label
            char label[8];
            std::snprintf(label, sizeof(label), "%.1f", tick_val);
            ImVec2 text_size = ImGui::CalcTextSize(label);
            draw_list->AddText(
                ImVec2(x - text_size.x * 0.5f, y_bottom + 2.0f),
                IM_COL32(120, 120, 120, 255), label);
        }

        // Add spacing to account for tick marks and labels
        ImGui::Dummy(ImVec2(0, 20.0f));
    }

    ImGui::Spacing();

    // Current value display
    ImGui::Text("Current: %.2f", gamma_slider_);

    // Gamma curve visualization
    ImGui::Spacing();
    ImGui::TextUnformatted("Gamma Curve");

    // Draw area for the curve
    const float curve_height = 100.0f;
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImVec2(ImGui::GetContentRegionAvail().x, curve_height);

    // Background
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(canvas_pos,
        ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
        IM_COL32(30, 30, 30, 255));

    // Draw linear reference line (gamma = 1.0)
    draw_list->AddLine(
        canvas_pos,
        ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
        IM_COL32(80, 80, 80, 255), 1.0f);

    // Draw gamma curve
    const double gamma = static_cast<double>(gamma_slider_);
    ImVec2 prev_point = canvas_pos;
    for (int i = 1; i <= 255; ++i) {
        double normalized_in = i / 255.0;
        double normalized_out = std::pow(normalized_in, 1.0 / gamma);

        float x = canvas_pos.x + (i / 255.0f) * canvas_size.x;
        float y = canvas_pos.y + canvas_size.y - static_cast<float>(normalized_out) * canvas_size.y;

        ImVec2 point(x, y);
        draw_list->AddLine(prev_point, point, IM_COL32(100, 200, 100, 255), 2.0f);
        prev_point = point;
    }

    // Border
    draw_list->AddRect(canvas_pos,
        ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
        IM_COL32(60, 60, 60, 255));

    // Reserve space for the canvas
    ImGui::Dummy(canvas_size);

    ImGui::Spacing();
    ImGui::Spacing();

    // Reset button
    if (ImGui::Button("Reset to Default", ImVec2(-1, 0))) {
        app.resetGamma();
        // Slider will auto-sync on next frame
    }

    // Hotkey hint
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::TextDisabled("Hotkeys: Ctrl+Alt+Up/Down, Ctrl+Alt+R");

    // Status at bottom
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("%s", app.getStatusText());
}

void MainWindow::renderHelpTab()
{
    ImGui::Spacing();

    ImGui::TextUnformatted("Usage");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextWrapped(
        "Use the slider to adjust your monitor's gamma value. "
        "Gamma affects the brightness and contrast of your display.");

    ImGui::Spacing();
    ImGui::BulletText("Values < 1.0: Darker image");
    ImGui::BulletText("Value = 1.0: Normal (default)");
    ImGui::BulletText("Values > 1.0: Brighter image");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextUnformatted("Global Hotkeys");
    ImGui::Spacing();

    ImGui::Columns(2, "hotkeys", false);
    ImGui::SetColumnWidth(0, 150);
    ImGui::Text("Ctrl+Alt+Up"); ImGui::NextColumn();
    ImGui::Text("Increase gamma"); ImGui::NextColumn();
    ImGui::Text("Ctrl+Alt+Down"); ImGui::NextColumn();
    ImGui::Text("Decrease gamma"); ImGui::NextColumn();
    ImGui::Text("Ctrl+Alt+R"); ImGui::NextColumn();
    ImGui::Text("Reset to default"); ImGui::NextColumn();
    ImGui::Columns(1);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextUnformatted("Command Line");
    ImGui::Spacing();
    ImGui::TextDisabled("lumos              Open the GUI");
    ImGui::TextDisabled("lumos 1.2          Set gamma to 1.2 and exit");
    ImGui::TextDisabled("lumos --help       Show help");
    ImGui::TextDisabled("lumos --version    Show version");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextUnformatted("Visualization Tools");
    ImGui::Spacing();
    ImGui::BulletText("Gamma Curve: Real-time visualization of gamma correction");
    ImGui::BulletText("Tick Marks: Common gamma values marked on slider");
    ImGui::BulletText("Test Pattern: B&W stripes for calibration (Help menu)");
}

void MainWindow::renderAboutTab()
{
    ImGui::Spacing();

    ImGui::TextUnformatted("Lumos v0.3.0");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextWrapped(
        "A modern C++ reimplementation of Gamminator, "
        "a monitor gamma adjustment utility for Windows.");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextUnformatted("Attribution:");
    ImGui::BulletText("Original Gamminator by Wolfgang Freiler (2005)");
    ImGui::BulletText("Multi-monitor mod by Lady Eklipse (v0.5.7)");
    ImGui::BulletText("Lumos reimplementation by Ian Dirk Armstrong");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextUnformatted("License: GPL v2");
    ImGui::TextDisabled("This is free software. You may redistribute copies of it");
    ImGui::TextDisabled("under the terms of the GNU General Public License.");
}

void MainWindow::renderTestPatternTab()
{
    ImGui::Spacing();

    ImGui::TextWrapped(
        "This pattern helps calibrate your display. "
        "Adjust gamma until the gray areas appear uniform in brightness.");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Draw striped test pattern
    const float pattern_height = 180.0f;
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImVec2(ImGui::GetContentRegionAvail().x, pattern_height);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Number of stripe pairs
    const int num_stripes = 16;
    const float stripe_width = canvas_size.x / num_stripes;

    for (int i = 0; i < num_stripes; ++i) {
        float x = canvas_pos.x + i * stripe_width;
        ImU32 color = (i % 2 == 0) ? IM_COL32(0, 0, 0, 255) : IM_COL32(255, 255, 255, 255);
        draw_list->AddRectFilled(
            ImVec2(x, canvas_pos.y),
            ImVec2(x + stripe_width, canvas_pos.y + canvas_size.y),
            color);
    }

    // Border
    draw_list->AddRect(canvas_pos,
        ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
        IM_COL32(100, 100, 100, 255));

    ImGui::Dummy(canvas_size);

    ImGui::Spacing();
    ImGui::TextDisabled("At correct gamma, alternating stripes should appear evenly bright.");
}

} // namespace lumos::ui
