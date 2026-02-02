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

        // Sync transfer function dropdown with app state
        using TF = lumos::platform::TransferFunction;
        TF current_tf = app.getTransferFunction();
        switch (current_tf) {
            case TF::Power: transfer_function_index_ = 0; break;
            case TF::sRGB: transfer_function_index_ = 1; break;
            case TF::Rec709:
            case TF::Rec2020: transfer_function_index_ = 2; break;
            case TF::DCIP3: transfer_function_index_ = 3; break;
            case TF::Custom: transfer_function_index_ = 4; break;
        }
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
        if (ImGui::BeginTabItem("Gamma")) {
            renderGammaTab(app);
            ImGui::EndTabItem();
        }

        // Help tab (closable)
        if (show_help_tab_) {
            if (ImGui::BeginTabItem("Help", &show_help_tab_)) {
                renderHelpTab();
                ImGui::EndTabItem();
            }
        }

        // About tab (closable)
        if (show_about_tab_) {
            if (ImGui::BeginTabItem("About", &show_about_tab_)) {
                renderAboutTab();
                ImGui::EndTabItem();
            }
        }

        ImGui::EndTabBar();
    }

    ImGui::End();

    // Render test pattern as a separate window (so it can be viewed while adjusting gamma)
    if (show_test_pattern_window_) {
        renderTestPatternWindow();
    }
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
                show_test_pattern_window_ = true;
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void MainWindow::renderGammaTab(App& app)
{
    ImGui::Spacing();

    // Transfer function selector
    ImGui::TextUnformatted("Transfer Function");
    const char* transfer_functions[] = {
        "Power (Custom Gamma)",
        "sRGB",
        "Rec.709 / Rec.2020",
        "DCI-P3 (Gamma 2.6)",
        "Custom (Edit Curve)"
    };

    ImGui::SetNextItemWidth(-1);
    if (ImGui::Combo("##TransferFunc", &transfer_function_index_, transfer_functions, IM_ARRAYSIZE(transfer_functions))) {
        // Map index to TransferFunction enum
        using TF = lumos::platform::TransferFunction;
        TF func;
        switch (transfer_function_index_) {
            case 1: func = TF::sRGB; break;
            case 2: func = TF::Rec709; break;
            case 3: func = TF::DCIP3; break;
            case 4: func = TF::Custom; break;
            case 0:
            default: func = TF::Power; break;
        }
        app.setTransferFunction(func);

        // Initialize UI curve points when switching to Custom mode
        if (func == TF::Custom && ui_curve_points_.empty()) {
            ui_curve_points_ = app.getCustomCurvePoints();
        }
    }

    ImGui::Spacing();

    // Gamma slider (only enabled for Power mode, disabled for preset and custom modes)
    bool is_power_mode = (transfer_function_index_ == 0);
    bool is_custom_mode = (transfer_function_index_ == 4);

    if (!is_power_mode) {
        ImGui::BeginDisabled();
    }

    ImGui::TextUnformatted("Gamma");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Absolute gamma value (not relative)");
    }
    ImGui::SetNextItemWidth(-1);
    bool slider_changed = ImGui::SliderFloat("##Gamma", &gamma_slider_, 0.1f, 9.0f, "%.2f");

    // Snap to common values when close (sticky tick marks)
    static bool was_dragging = false;
    bool is_dragging = ImGui::IsItemActive();

    if (was_dragging && !is_dragging) {  // Just released the slider
        const float tick_values[] = { 1.0f, 1.8f, 2.2f, 2.5f };
        const float snap_threshold = 0.05f;  // Snap within ±0.05

        for (float tick_val : tick_values) {
            if (std::fabs(gamma_slider_ - tick_val) < snap_threshold) {
                gamma_slider_ = tick_val;
                slider_changed = true;  // Ensure we apply the snapped value
                break;
            }
        }
    }
    was_dragging = is_dragging;

    if (slider_changed) {
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

    // Numeric input for precise control
    ImGui::TextUnformatted("Precise Value:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::InputFloat("##GammaInput", &gamma_slider_, 0.0f, 0.0f, "%.2f")) {
        // Clamp to valid range
        if (gamma_slider_ < 0.1f) gamma_slider_ = 0.1f;
        if (gamma_slider_ > 9.0f) gamma_slider_ = 9.0f;
        app.setGamma(static_cast<double>(gamma_slider_));
    }

    if (!is_power_mode) {
        ImGui::EndDisabled();
    }

    ImGui::Spacing();

    // Custom curve editor controls (only show in Custom mode)
    if (is_custom_mode) {
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextUnformatted("Custom Curve Editor");
        ImGui::TextDisabled("Ctrl+Click: Add point | Drag: Move point | Right-click: Delete (middle points only)");

        if (ImGui::Button("Reset to Linear", ImVec2(-1, 0))) {
            ui_curve_points_ = {{0.0, 0.0}, {1.0, 1.0}};
            app.setCustomCurvePoints(ui_curve_points_);
        }
        ImGui::Spacing();
    }

    // Gamma curve visualization
    ImGui::Spacing();
    ImGui::TextUnformatted("Gamma Curve");

    // Draw area for the curve (with margins for axis labels)
    const float total_height = 200.0f;
    const float left_margin = 45.0f;
    const float bottom_margin = 35.0f;
    const float right_margin = 10.0f;
    const float top_margin = 10.0f;

    ImVec2 total_canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 total_canvas_size = ImVec2(ImGui::GetContentRegionAvail().x, total_height);

    // Actual curve area (excluding margins)
    ImVec2 canvas_pos = ImVec2(total_canvas_pos.x + left_margin, total_canvas_pos.y + top_margin);
    ImVec2 canvas_size = ImVec2(total_canvas_size.x - left_margin - right_margin,
                                 total_canvas_size.y - top_margin - bottom_margin);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Background for entire area
    draw_list->AddRectFilled(total_canvas_pos,
        ImVec2(total_canvas_pos.x + total_canvas_size.x, total_canvas_pos.y + total_canvas_size.y),
        IM_COL32(25, 25, 25, 255));

    // Background for curve area
    draw_list->AddRectFilled(canvas_pos,
        ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
        IM_COL32(30, 30, 30, 255));

    // Draw grid lines and axis marks
    const float grid_values[] = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };
    for (float val : grid_values) {
        // Vertical grid lines (for X values)
        float x = canvas_pos.x + val * canvas_size.x;
        draw_list->AddLine(
            ImVec2(x, canvas_pos.y),
            ImVec2(x, canvas_pos.y + canvas_size.y),
            val == 0.5f ? IM_COL32(60, 60, 60, 255) : IM_COL32(45, 45, 45, 255),
            val == 0.5f ? 1.5f : 1.0f);

        // X-axis tick marks and labels
        draw_list->AddLine(
            ImVec2(x, canvas_pos.y + canvas_size.y),
            ImVec2(x, canvas_pos.y + canvas_size.y + 5.0f),
            IM_COL32(150, 150, 150, 255), 1.5f);

        char label[8];
        std::snprintf(label, sizeof(label), "%.2f", val);
        ImVec2 text_size = ImGui::CalcTextSize(label);
        draw_list->AddText(
            ImVec2(x - text_size.x * 0.5f, canvas_pos.y + canvas_size.y + 8.0f),
            IM_COL32(180, 180, 180, 255), label);

        // Horizontal grid lines (for Y values)
        float y = canvas_pos.y + canvas_size.y - val * canvas_size.y;
        draw_list->AddLine(
            ImVec2(canvas_pos.x, y),
            ImVec2(canvas_pos.x + canvas_size.x, y),
            val == 0.5f ? IM_COL32(60, 60, 60, 255) : IM_COL32(45, 45, 45, 255),
            val == 0.5f ? 1.5f : 1.0f);

        // Y-axis tick marks and labels
        draw_list->AddLine(
            ImVec2(canvas_pos.x - 5.0f, y),
            ImVec2(canvas_pos.x, y),
            IM_COL32(150, 150, 150, 255), 1.5f);

        std::snprintf(label, sizeof(label), "%.2f", val);
        text_size = ImGui::CalcTextSize(label);
        draw_list->AddText(
            ImVec2(canvas_pos.x - text_size.x - 8.0f, y - text_size.y * 0.5f),
            IM_COL32(180, 180, 180, 255), label);
    }

    // Axis labels
    const char* x_label = "Encoded Signal";
    const char* y_label = "Luminance";
    ImVec2 x_label_size = ImGui::CalcTextSize(x_label);

    // X-axis label (centered below graph)
    draw_list->AddText(
        ImVec2(canvas_pos.x + canvas_size.x * 0.5f - x_label_size.x * 0.5f,
               canvas_pos.y + canvas_size.y + 25.0f),
        IM_COL32(200, 200, 200, 255), x_label);

    // Y-axis label (rotated 90 degrees counterclockwise)
    ImVec2 y_label_size = ImGui::CalcTextSize(y_label);

    // Start position for horizontal text (before rotation)
    ImVec2 y_label_start = ImVec2(
        total_canvas_pos.x + 10.0f,
        canvas_pos.y + canvas_size.y * 0.5f - y_label_size.x * 0.5f
    );

    // Record vertex count before adding text
    int vtx_count_before = draw_list->VtxBuffer.Size;

    // Add text horizontally (will be rotated)
    draw_list->AddText(y_label_start, IM_COL32(200, 200, 200, 255), y_label);

    // Rotate vertices 90 degrees counterclockwise around text center
    int vtx_count_after = draw_list->VtxBuffer.Size;
    if (vtx_count_after > vtx_count_before) {
        // Center point of the text for rotation
        ImVec2 center = ImVec2(
            y_label_start.x + y_label_size.x * 0.5f,
            y_label_start.y + y_label_size.y * 0.5f
        );

        for (int i = vtx_count_before; i < vtx_count_after; i++) {
            ImDrawVert& v = draw_list->VtxBuffer[i];
            float x = v.pos.x - center.x;
            float y = v.pos.y - center.y;
            v.pos.x = center.x - y;  // 90° counterclockwise rotation
            v.pos.y = center.y + x;
        }
    }

    // Draw linear reference line (gamma = 1.0)
    draw_list->AddLine(
        canvas_pos,
        ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
        IM_COL32(80, 80, 80, 255), 1.5f);

    // Draw gamma curve
    const double gamma = static_cast<double>(gamma_slider_);

    // Calculate the starting point at x=0
    double start_y = 0.0;
    if (is_custom_mode && !ui_curve_points_.empty()) {
        start_y = ui_curve_points_.front().y;
    } else {
        // For all standard transfer functions, f(0) = 0
        start_y = 0.0;
    }
    ImVec2 prev_point(canvas_pos.x, canvas_pos.y + canvas_size.y - static_cast<float>(start_y) * canvas_size.y);

    // Initialize ui_curve_points if in custom mode and not yet initialized
    if (is_custom_mode && ui_curve_points_.empty()) {
        ui_curve_points_ = app.getCustomCurvePoints();
    }

    // Sort ui_curve_points by x before drawing
    if (is_custom_mode && !ui_curve_points_.empty()) {
        std::sort(ui_curve_points_.begin(), ui_curve_points_.end());
    }

    for (int i = 1; i <= 255; ++i) {
        double linear = i / 255.0;
        double normalized_out;

        // Apply the appropriate transfer function
        if (is_custom_mode && !ui_curve_points_.empty()) {
            // Use custom curve
            const auto& points = ui_curve_points_;

            if (linear <= points.front().x) {
                normalized_out = points.front().y;
            }
            else if (linear >= points.back().x) {
                normalized_out = points.back().y;
            }
            else {
                // Linear interpolation
                for (size_t j = 0; j < points.size() - 1; ++j) {
                    if (linear >= points[j].x && linear <= points[j + 1].x) {
                        double x1 = points[j].x;
                        double y1 = points[j].y;
                        double x2 = points[j + 1].x;
                        double y2 = points[j + 1].y;
                        double t = (linear - x1) / (x2 - x1);
                        normalized_out = y1 + t * (y2 - y1);
                        break;
                    }
                }
            }
        }
        else {
            switch (transfer_function_index_) {
                case 1: { // sRGB
                    if (linear <= 0.0031308)
                        normalized_out = 12.92 * linear;
                    else
                        normalized_out = 1.055 * std::pow(linear, 1.0 / 2.4) - 0.055;
                    break;
                }
                case 2: { // Rec.709
                    if (linear < 0.018)
                        normalized_out = 4.5 * linear;
                    else
                        normalized_out = 1.099 * std::pow(linear, 0.45) - 0.099;
                    break;
                }
                case 3: { // DCI-P3
                    normalized_out = std::pow(linear, 1.0 / 2.6);
                    break;
                }
                case 0: // Power
                default:
                    normalized_out = std::pow(linear, 1.0 / gamma);
                    break;
            }
        }

        float x = canvas_pos.x + (i / 255.0f) * canvas_size.x;
        float y = canvas_pos.y + canvas_size.y - static_cast<float>(normalized_out) * canvas_size.y;

        ImVec2 point(x, y);
        draw_list->AddLine(prev_point, point, IM_COL32(100, 200, 100, 255), 2.0f);
        prev_point = point;
    }

    // Interactive control points (only in Custom mode)
    if (is_custom_mode) {
        const float point_radius = 6.0f;
        const float click_radius = 8.0f;

        // Get mouse state
        ImGuiIO& io = ImGui::GetIO();
        ImVec2 mouse_pos = io.MousePos;
        bool mouse_in_canvas = (mouse_pos.x >= canvas_pos.x && mouse_pos.x <= canvas_pos.x + canvas_size.x &&
                                mouse_pos.y >= canvas_pos.y && mouse_pos.y <= canvas_pos.y + canvas_size.y);

        // Handle dragging
        if (dragging_point_ && selected_point_index_ >= 0 && selected_point_index_ < static_cast<int>(ui_curve_points_.size())) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                // Update point position
                double new_x = (mouse_pos.x - canvas_pos.x) / canvas_size.x;
                double new_y = 1.0 - (mouse_pos.y - canvas_pos.y) / canvas_size.y;

                // Clamp to [0, 1]
                new_x = std::clamp(new_x, 0.0, 1.0);
                new_y = std::clamp(new_y, 0.0, 1.0);

                // First point is locked to x=0, last point is locked to x=1
                bool is_first_point = (selected_point_index_ == 0);
                bool is_last_point = (selected_point_index_ == static_cast<int>(ui_curve_points_.size()) - 1);

                if (is_first_point) {
                    new_x = 0.0;
                } else if (is_last_point) {
                    new_x = 1.0;
                }

                ui_curve_points_[selected_point_index_].x = new_x;
                ui_curve_points_[selected_point_index_].y = new_y;

                // Apply changes immediately
                app.setCustomCurvePoints(ui_curve_points_);
            }
            else {
                // Mouse released, stop dragging
                dragging_point_ = false;
                selected_point_index_ = -1;
            }
        }

        // Render control points and handle clicks
        int hovered_point = -1;
        for (size_t i = 0; i < ui_curve_points_.size(); ++i) {
            float px = canvas_pos.x + static_cast<float>(ui_curve_points_[i].x) * canvas_size.x;
            float py = canvas_pos.y + canvas_size.y - static_cast<float>(ui_curve_points_[i].y) * canvas_size.y;

            // Check if mouse is hovering this point
            float dx = mouse_pos.x - px;
            float dy = mouse_pos.y - py;
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist <= click_radius && mouse_in_canvas) {
                hovered_point = static_cast<int>(i);
            }

            // Draw point
            bool is_selected = (static_cast<int>(i) == selected_point_index_);
            bool is_hovered = (static_cast<int>(i) == hovered_point);

            ImU32 point_color = is_selected ? IM_COL32(255, 200, 100, 255) :
                                is_hovered ? IM_COL32(150, 220, 150, 255) :
                                IM_COL32(100, 200, 100, 255);

            draw_list->AddCircleFilled(ImVec2(px, py), point_radius, point_color);
            draw_list->AddCircle(ImVec2(px, py), point_radius, IM_COL32(50, 50, 50, 255), 0, 2.0f);
        }

        // Handle mouse clicks
        if (mouse_in_canvas && !dragging_point_) {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                if (hovered_point >= 0) {
                    // Start dragging existing point
                    selected_point_index_ = hovered_point;
                    dragging_point_ = true;
                }
                else if (io.KeyCtrl) {
                    // Add new point (only with Ctrl held)
                    double new_x = (mouse_pos.x - canvas_pos.x) / canvas_size.x;
                    double new_y = 1.0 - (mouse_pos.y - canvas_pos.y) / canvas_size.y;
                    new_x = std::clamp(new_x, 0.0, 1.0);
                    new_y = std::clamp(new_y, 0.0, 1.0);

                    ui_curve_points_.push_back({new_x, new_y});
                    std::sort(ui_curve_points_.begin(), ui_curve_points_.end());
                    app.setCustomCurvePoints(ui_curve_points_);
                }
            }
            else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && hovered_point >= 0) {
                // Delete point (keep minimum 2 points, never delete first or last)
                bool is_first = (hovered_point == 0);
                bool is_last = (hovered_point == static_cast<int>(ui_curve_points_.size()) - 1);
                if (ui_curve_points_.size() > 2 && !is_first && !is_last) {
                    ui_curve_points_.erase(ui_curve_points_.begin() + hovered_point);
                    app.setCustomCurvePoints(ui_curve_points_);
                    selected_point_index_ = -1;
                }
            }
        }

        // Show tooltip with coordinates when hovering a point
        if (hovered_point >= 0) {
            ImGui::BeginTooltip();
            ImGui::Text("X: %.3f, Y: %.3f", ui_curve_points_[hovered_point].x, ui_curve_points_[hovered_point].y);
            ImGui::EndTooltip();
        }
    }

    // Border around curve area
    draw_list->AddRect(canvas_pos,
        ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
        IM_COL32(60, 60, 60, 255));

    // Reserve space for the entire canvas (including margins)
    ImGui::Dummy(total_canvas_size);

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

void MainWindow::renderTestPatternWindow()
{
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Test Pattern", &show_test_pattern_window_, ImGuiWindowFlags_None)) {
        ImGui::TextWrapped(
            "This pattern helps calibrate your display. "
            "Keep this window open while adjusting the gamma slider.");

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
            "Note: This window is gamma-corrected by your current settings.");
        ImGui::TextWrapped(
            "The stripes should appear to blend into uniform gray at the correct gamma. "
            "If they appear banded or one color dominates, adjust the gamma.");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Draw striped test pattern
        const float pattern_height = ImGui::GetContentRegionAvail().y - 20.0f;
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_size = ImVec2(ImGui::GetContentRegionAvail().x, pattern_height);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Number of stripe pairs - more stripes for better blending test
        const int num_stripes = 32;
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
    }
    ImGui::End();
}

} // namespace lumos::ui
