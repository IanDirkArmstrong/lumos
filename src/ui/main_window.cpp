// Lumos - Main UI window
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#include "main_window.h"
#include "../app.h"
#include "imgui.h"
#include "implot.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace lumos::ui {

void MainWindow::render(App& app)
{
    // Always sync slider with app state (in case it was changed by hotkeys or tray menu)
    // Only update if not actively being dragged to avoid fighting with user input
    float current_gamma = static_cast<float>(app.getGamma());
    if (!ImGui::IsAnyItemActive() || first_frame_) {
        gamma_slider_ = current_gamma;

        // Sync tone curve dropdown with app state
        using TC = lumos::platform::ToneCurve;
        TC current_tc = app.getToneCurve();
        switch (current_tc) {
            case TC::Linear: transfer_function_index_ = 0; break;
            case TC::Power: transfer_function_index_ = 1; break;
            case TC::ShadowLift: transfer_function_index_ = 2; break;
            case TC::SoftContrast: transfer_function_index_ = 3; break;
            case TC::Cinema: transfer_function_index_ = 4; break;
            case TC::Custom: transfer_function_index_ = 5; break;
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
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_MenuBar;

    ImGui::Begin("##MainWindow", nullptr, flags);

    // Render menu bar
    renderMenuBar(app);

    // Calculate content area height (excluding status bar)
    const float status_bar_height = 24.0f;
    float content_height = ImGui::GetContentRegionAvail().y - status_bar_height;

    // Content area child window (scrollable, but status bar stays fixed below)
    if (ImGui::BeginChild("##ContentArea", ImVec2(0, content_height), false, ImGuiWindowFlags_None)) {
        // Render tab bar
        if (ImGui::BeginTabBar("MainTabBar", ImGuiTabBarFlags_None)) {
            // Gamma control tab (always visible, not closable)
            if (ImGui::BeginTabItem("Gamma")) {
                renderGammaTab(app);
                ImGui::EndTabItem();
            }

            // Help tab (closable)
            if (show_help_tab_) {
                ImGuiTabItemFlags help_flags = focus_help_tab_ ? ImGuiTabItemFlags_SetSelected : 0;
                if (ImGui::BeginTabItem("Help", &show_help_tab_, help_flags)) {
                    renderHelpTab();
                    ImGui::EndTabItem();
                }
                focus_help_tab_ = false;
            }

            // About tab (closable)
            if (show_about_tab_) {
                ImGuiTabItemFlags about_flags = focus_about_tab_ ? ImGuiTabItemFlags_SetSelected : 0;
                if (ImGui::BeginTabItem("About", &show_about_tab_, about_flags)) {
                    renderAboutTab();
                    ImGui::EndTabItem();
                }
                focus_about_tab_ = false;
            }

            // Settings tab (closable)
            if (show_settings_tab_) {
                ImGuiTabItemFlags settings_flags = focus_settings_tab_ ? ImGuiTabItemFlags_SetSelected : 0;
                if (ImGui::BeginTabItem("Settings", &show_settings_tab_, settings_flags)) {
                    renderSettingsTab(app);
                    ImGui::EndTabItem();
                }
                focus_settings_tab_ = false;
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::EndChild();

    // Render status bar at the bottom
    renderStatusBar(app);

    ImGui::End();

    // Render test pattern as a separate window (so it can be viewed while adjusting gamma)
    if (show_test_pattern_window_) {
        renderTestPatternWindow();
    }
}

void MainWindow::renderMenuBar(App& app)
{
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Close to Tray")) {
                // Hide window to tray
                app.hideWindow();
            }
            if (ImGui::MenuItem("Exit")) {
                // Request actual exit
                app.requestExit();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Settings")) {
                show_settings_tab_ = true;
                focus_settings_tab_ = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Help", "F1")) {
                show_help_tab_ = true;
                focus_help_tab_ = true;
            }
            if (ImGui::MenuItem("About")) {
                show_about_tab_ = true;
                focus_about_tab_ = true;
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

static void drawMonitorIcon(ImDrawList* draw_list, ImVec2 pos, float size, bool active)
{
    // Colors - outline style (active = green outline, inactive = gray outline)
    ImU32 outline_color = active ? IM_COL32(100, 200, 120, 255) : IM_COL32(120, 120, 120, 255);
    float thickness = 1.5f;

    // Monitor body (outer rectangle) - outline only
    float body_width = size * 0.85f;
    float body_height = size * 0.6f;
    float body_x = pos.x + (size - body_width) * 0.5f;
    float body_y = pos.y;

    draw_list->AddRect(
        ImVec2(body_x, body_y),
        ImVec2(body_x + body_width, body_y + body_height),
        outline_color, 2.0f, 0, thickness);

    // Screen (inner rectangle) - outline only
    float screen_margin = size * 0.1f;
    draw_list->AddRect(
        ImVec2(body_x + screen_margin, body_y + screen_margin),
        ImVec2(body_x + body_width - screen_margin, body_y + body_height - screen_margin),
        outline_color, 1.0f, 0, thickness * 0.7f);

    // Stand neck - filled (small detail)
    float neck_width = size * 0.12f;
    float neck_height = size * 0.1f;
    float neck_x = pos.x + (size - neck_width) * 0.5f;
    float neck_y = body_y + body_height;

    draw_list->AddRectFilled(
        ImVec2(neck_x, neck_y),
        ImVec2(neck_x + neck_width, neck_y + neck_height),
        outline_color);

    // Stand base - outline
    float base_width = size * 0.35f;
    float base_height = size * 0.06f;
    float base_x = pos.x + (size - base_width) * 0.5f;
    float base_y = neck_y + neck_height;

    draw_list->AddRectFilled(
        ImVec2(base_x, base_y),
        ImVec2(base_x + base_width, base_y + base_height),
        outline_color, 1.0f);
}

void MainWindow::renderStatusBar(App& app)
{
    // Position status bar at the bottom of the window
    float status_height = 24.0f;
    ImVec2 window_pos = ImGui::GetWindowPos();
    ImVec2 window_size = ImGui::GetWindowSize();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Calculate status bar position (at bottom of window, edge to edge)
    float status_y = window_pos.y + window_size.y - status_height;

    // Draw background fill for status bar (edge to edge)
    draw_list->AddRectFilled(
        ImVec2(window_pos.x, status_y),
        ImVec2(window_pos.x + window_size.x, window_pos.y + window_size.y),
        IM_COL32(30, 30, 30, 255));

    // Draw separator line (edge to edge)
    draw_list->AddLine(
        ImVec2(window_pos.x, status_y),
        ImVec2(window_pos.x + window_size.x, status_y),
        IM_COL32(60, 60, 60, 255), 1.0f);

    // Layout parameters
    float padding_x = ImGui::GetStyle().WindowPadding.x;
    float icon_size = 16.0f;
    float icon_padding = 4.0f;
    float vertical_padding = (status_height - icon_size) * 0.5f;
    float content_y = status_y + vertical_padding;

    // Get preset name
    const char* preset_names[] = {
        "Neutral", "Simple Gamma", "Shadow Lift", "Soft Contrast", "Cinema", "Custom"
    };
    const char* preset_name = preset_names[transfer_function_index_];

    // Draw preset text on the left
    float text_x = window_pos.x + padding_x;
    float text_y = status_y + (status_height - ImGui::GetTextLineHeight()) * 0.5f;
    draw_list->AddText(ImVec2(text_x, text_y), IM_COL32(180, 180, 180, 255), preset_name);

    // Monitor icons on the right
    size_t monitor_count = app.getMonitorCount();
    bool gamma_active = app.isGammaEnabled();
    float icons_total_width = monitor_count * (icon_size + icon_padding) - icon_padding;
    float icons_x = window_pos.x + window_size.x - padding_x - icons_total_width;

    // Draw vertical separator before icons
    float separator_x = icons_x - 8.0f;
    draw_list->AddLine(
        ImVec2(separator_x, status_y + 4.0f),
        ImVec2(separator_x, status_y + status_height - 4.0f),
        IM_COL32(60, 60, 60, 255), 1.0f);

    // Draw monitor icons
    for (size_t i = 0; i < monitor_count; ++i) {
        ImVec2 icon_pos(icons_x + i * (icon_size + icon_padding), content_y);
        drawMonitorIcon(draw_list, icon_pos, icon_size, gamma_active);
    }

    // Set cursor position for invisible button (for tooltip interaction on icons)
    ImGui::SetCursorScreenPos(ImVec2(icons_x, content_y));
    float icons_width = (monitor_count > 0) ? icons_total_width : 1.0f;
    ImGui::InvisibleButton("##MonitorStatus", ImVec2(icons_width, icon_size));

    if (ImGui::IsItemHovered()) {
        if (gamma_active) {
            ImGui::SetTooltip("Applied to %zu display%s", monitor_count, monitor_count == 1 ? "" : "s");
        } else {
            ImGui::SetTooltip("Gamma disabled (%zu display%s)", monitor_count, monitor_count == 1 ? "" : "s");
        }
    }
}

void MainWindow::renderGammaTab(App& app)
{
    ImGui::Spacing();

    // Tone curve preset selector
    ImGui::TextUnformatted("Tone Curve Preset");
    const char* tone_curves[] = {
        "Neutral (Identity)",
        "Simple Gamma",
        "Shadow Lift",
        "Soft Contrast",
        "Cinema (Gamma 2.6)",
        "Custom (Edit Curve)"
    };

    ImGui::SetNextItemWidth(-1);
    if (ImGui::Combo("##ToneCurve", &transfer_function_index_, tone_curves, IM_ARRAYSIZE(tone_curves))) {
        // Map index to ToneCurve enum
        using TC = lumos::platform::ToneCurve;
        TC curve;
        switch (transfer_function_index_) {
            case 0: curve = TC::Linear; break;
            case 2: curve = TC::ShadowLift; break;
            case 3: curve = TC::SoftContrast; break;
            case 4: curve = TC::Cinema; break;
            case 5: curve = TC::Custom; break;
            case 1:
            default: curve = TC::Power; break;
        }
        app.setToneCurve(curve);

        // Initialize UI curve points when switching to Custom mode
        if (curve == TC::Custom && ui_curve_points_.empty()) {
            ui_curve_points_ = app.getCustomCurvePoints();
        }
    }

    ImGui::Spacing();

    // Determine which mode we're in
    bool is_power_mode = (transfer_function_index_ == 1);  // Simple Gamma
    bool is_custom_mode = (transfer_function_index_ == 5); // Custom

    // Curve strength slider and precise value input (only visible for Simple Gamma mode)
    if (is_power_mode) {
        ImGui::TextUnformatted("Curve Strength");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Controls the power-law exponent");
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

        ImGui::Spacing();
    }

    // Custom curve editor controls (only show in Custom mode)
    if (is_custom_mode) {
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextUnformatted("Custom Curve Editor");
        ImGui::TextDisabled("Ctrl+Click: Add point | Drag: Move point | Right-click: Delete (middle points only)");

        // Capture a reference curve the first time we enter custom mode this session
        if (reference_curve_points_.empty()) {
            reference_curve_points_ = ui_curve_points_;
        }

        if (ImGui::Button("Reset to Linear", ImVec2(-1, 0))) {
            ui_curve_points_ = {{0.0, 0.0}, {1.0, 1.0}};
            app.setCustomCurvePoints(ui_curve_points_);
            // Keep the original reference curve untouched so it remains a baseline
        }
        ImGui::Spacing();
    }

    // Tone curve visualization
    ImGui::Spacing();
    ImGui::TextUnformatted("Output Curve Preview");

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
    const char* x_label = "Input (0-1)";
    const char* y_label = "Output (0-1)";
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

    // Draw screen histogram as background (if enabled)
    if (show_histogram_) {
        auto histogram = app.getScreenHistogram();

        // Ensure custom curve is sorted before reuse
        if (is_custom_mode && !ui_curve_points_.empty()) {
            std::sort(ui_curve_points_.begin(), ui_curve_points_.end());
        }
        if (is_custom_mode && !reference_curve_points_.empty()) {
            std::sort(reference_curve_points_.begin(), reference_curve_points_.end());
        }

        // Helper to evaluate the active tone curve at [0,1]
        auto eval_curve = [&](double linear) -> double {
            if (is_custom_mode && !ui_curve_points_.empty()) {
                const auto& points = ui_curve_points_;
                if (linear <= points.front().x) return points.front().y;
                if (linear >= points.back().x) return points.back().y;
                for (size_t j = 0; j < points.size() - 1; ++j) {
                    if (linear >= points[j].x && linear <= points[j + 1].x) {
                        double x1 = points[j].x, y1 = points[j].y;
                        double x2 = points[j + 1].x, y2 = points[j + 1].y;
                        double t = (linear - x1) / (x2 - x1);
                        return y1 + t * (y2 - y1);
                    }
                }
                return linear;
            }

            switch (transfer_function_index_) {
                case 0: // Neutral
                    return linear;
                case 2: // Shadow Lift
                    return (linear <= 0.0031308)
                        ? 12.92 * linear
                        : 1.055 * std::pow(linear, 1.0 / 2.4) - 0.055;
                case 3: // Soft Contrast
                    return (linear < 0.018)
                        ? 4.5 * linear
                        : 1.099 * std::pow(linear, 0.45) - 0.099;
                case 4: // Cinema
                    return std::pow(linear, 1.0 / 2.6);
                case 1: // Simple Gamma
                default:
                    return std::pow(linear, 1.0 / static_cast<double>(gamma_slider_));
            }
        };
        // Helper to evaluate the reference curve (if present)
        auto eval_reference = [&](double linear) -> double {
            if (reference_curve_points_.empty()) return linear;
            const auto& points = reference_curve_points_;
            if (linear <= points.front().x) return points.front().y;
            if (linear >= points.back().x) return points.back().y;
            for (size_t j = 0; j < points.size() - 1; ++j) {
                if (linear >= points[j].x && linear <= points[j + 1].x) {
                    double x1 = points[j].x, y1 = points[j].y;
                    double x2 = points[j + 1].x, y2 = points[j + 1].y;
                    double t = (linear - x1) / (x2 - x1);
                    return y1 + t * (y2 - y1);
                }
            }
            return linear;
        };

        if (histogram.valid) {
            // Base (pre-LUT) histogram
            for (int i = 0; i < 256; ++i) {
                histogram_xs_[i] = i / 255.0f;
                histogram_ys_[i] = histogram.luminance[i];
                histogram_ys_post_[i] = 0.0f;
                histogram_diff_[i] = 0.0f;
            }

            // Build "post" histogram by remapping bin centers through the active curve
            for (int i = 0; i < 256; ++i) {
                double in_x = i / 255.0;
                float weight = histogram_ys_[i];
                double out_x = std::clamp(eval_curve(in_x), 0.0, 1.0);

                double pos = out_x * 255.0;
                int idx = static_cast<int>(pos);
                double t = pos - idx;

                if (idx >= 0 && idx < 256) {
                    histogram_ys_post_[idx] += static_cast<float>(weight * (1.0 - t));
                }
                if (idx + 1 < 256) {
                    histogram_ys_post_[idx + 1] += static_cast<float>(weight * t);
                }
            }

            // Normalize both histograms independently to preserve shape
            auto normalize = [](std::array<float, 256>& arr) {
                float m = 0.0f;
                for (float v : arr) m = (std::max)(m, v);
                if (m > 0.0f) {
                    for (float& v : arr) v /= m;
                }
            };
            normalize(histogram_ys_);
            normalize(histogram_ys_post_);

            // Diff = post - pre (normalized)
            float max_abs_diff = 0.0f;
            for (int i = 0; i < 256; ++i) {
                histogram_diff_[i] = histogram_ys_post_[i] - histogram_ys_[i];
                max_abs_diff = (std::max)(max_abs_diff, std::fabs(histogram_diff_[i]));
            }
            if (max_abs_diff > 0.0f) {
                for (float& v : histogram_diff_) v /= max_abs_diff;
            }

            // Draw histograms
            float bar_width = canvas_size.x / 256.0f;
            for (int i = 0; i < 256; ++i) {
                // Pre (blue)
                float h_pre = histogram_ys_[i] * canvas_size.y * 0.7f;
                float x_pre = canvas_pos.x + histogram_xs_[i] * canvas_size.x;
                draw_list->AddRectFilled(
                    ImVec2(x_pre, canvas_pos.y + canvas_size.y - h_pre),
                    ImVec2(x_pre + bar_width * 0.9f, canvas_pos.y + canvas_size.y),
                    IM_COL32(60, 90, 150, 70));

                // Post (amber), slightly narrower and inset
                float h_post = histogram_ys_post_[i] * canvas_size.y * 0.7f;
                float x_post = x_pre + bar_width * 0.15f;
                draw_list->AddRectFilled(
                    ImVec2(x_post, canvas_pos.y + canvas_size.y - h_post),
                    ImVec2(x_post + bar_width * 0.6f, canvas_pos.y + canvas_size.y),
                    IM_COL32(200, 140, 60, 80));

                // Diff (line), green for positive, red for negative
                float dy = histogram_diff_[i] * canvas_size.y * 0.25f; // ±25% height
                ImU32 diff_col = (histogram_diff_[i] >= 0.0f)
                    ? IM_COL32(80, 200, 120, 160)
                    : IM_COL32(220, 80, 80, 160);
                draw_list->AddLine(
                    ImVec2(x_pre + bar_width * 0.45f, canvas_pos.y + canvas_size.y),
                    ImVec2(x_pre + bar_width * 0.45f, canvas_pos.y + canvas_size.y - dy),
                    diff_col, 1.5f);
            }
        }
    }

    // Draw linear reference line (gamma = 1.0)
    draw_list->AddLine(
        canvas_pos,
        ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
        IM_COL32(80, 80, 80, 255), 1.5f);

    // Draw Windows API valid zone (50-150% of identity) - only in Custom mode
    if (is_custom_mode) {
        // Build polygon for the valid zone
        std::vector<ImVec2> valid_zone_top;
        std::vector<ImVec2> valid_zone_bottom;

        for (int i = 0; i <= 64; ++i) {
            double x_norm = i / 64.0;
            double min_y = x_norm * 0.5;
            double max_y = (std::min)(1.0, x_norm * 1.5 + 0.05);

            float px = canvas_pos.x + static_cast<float>(x_norm) * canvas_size.x;
            float py_min = canvas_pos.y + canvas_size.y - static_cast<float>(min_y) * canvas_size.y;
            float py_max = canvas_pos.y + canvas_size.y - static_cast<float>(max_y) * canvas_size.y;

            valid_zone_top.push_back(ImVec2(px, py_max));
            valid_zone_bottom.push_back(ImVec2(px, py_min));
        }

        // Draw as filled quad strips (valid zone in green tint)
        for (size_t i = 0; i < valid_zone_top.size() - 1; ++i) {
            draw_list->AddQuadFilled(
                valid_zone_top[i], valid_zone_top[i + 1],
                valid_zone_bottom[i + 1], valid_zone_bottom[i],
                IM_COL32(50, 80, 50, 40));
        }

        // Draw boundary lines for the valid zone
        for (size_t i = 0; i < valid_zone_top.size() - 1; ++i) {
            draw_list->AddLine(valid_zone_top[i], valid_zone_top[i + 1], IM_COL32(80, 120, 80, 100), 1.0f);
            draw_list->AddLine(valid_zone_bottom[i], valid_zone_bottom[i + 1], IM_COL32(80, 120, 80, 100), 1.0f);
        }
    }

    // Draw tone curve
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
    if (is_custom_mode && !reference_curve_points_.empty()) {
        std::sort(reference_curve_points_.begin(), reference_curve_points_.end());
    }

    // Reference curve overlay (dashed line) for custom mode
    if (is_custom_mode && !reference_curve_points_.empty()) {
        ImVec2 prev_ref(canvas_pos.x,
            canvas_pos.y + canvas_size.y - static_cast<float>(reference_curve_points_.front().y) * canvas_size.y);
        for (int i = 1; i <= 255; ++i) {
            double linear = i / 255.0;
            double ref_out = 0.0;
            // Reuse eval_reference from histogram block via lambda duplication
            auto eval_ref_local = [&](double l) {
                if (reference_curve_points_.empty()) return l;
                const auto& pts = reference_curve_points_;
                if (l <= pts.front().x) return pts.front().y;
                if (l >= pts.back().x) return pts.back().y;
                for (size_t j = 0; j < pts.size() - 1; ++j) {
                    if (l >= pts[j].x && l <= pts[j + 1].x) {
                        double t = (l - pts[j].x) / (pts[j + 1].x - pts[j].x);
                        return pts[j].y + t * (pts[j + 1].y - pts[j].y);
                    }
                }
                return l;
            };
            ref_out = eval_ref_local(linear);

            float x = canvas_pos.x + (i / 255.0f) * canvas_size.x;
            float y = canvas_pos.y + canvas_size.y - static_cast<float>(ref_out) * canvas_size.y;
            ImVec2 point(x, y);

            // Dashed effect: draw every other small segment
            if (i % 4 < 2) {
                draw_list->AddLine(prev_ref, point, IM_COL32(200, 200, 200, 120), 1.0f);
            }
            prev_ref = point;
        }
    }

    for (int i = 1; i <= 255; ++i) {
        double linear = i / 255.0;
        double normalized_out = linear;  // Default to identity

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
                case 0: // Neutral (Identity)
                    normalized_out = linear;
                    break;
                case 2: { // Shadow Lift
                    if (linear <= 0.0031308)
                        normalized_out = 12.92 * linear;
                    else
                        normalized_out = 1.055 * std::pow(linear, 1.0 / 2.4) - 0.055;
                    break;
                }
                case 3: { // Soft Contrast
                    if (linear < 0.018)
                        normalized_out = 4.5 * linear;
                    else
                        normalized_out = 1.099 * std::pow(linear, 0.45) - 0.099;
                    break;
                }
                case 4: // Cinema (Gamma 2.6)
                    normalized_out = std::pow(linear, 1.0 / 2.6);
                    break;
                case 1: // Simple Gamma
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

                // Snap to reference curve if close
                if (!reference_curve_points_.empty()) {
                    // Vertical snap to reference curve value at this X
                    // Find interpolated ref Y
                    double ref_y = 0.0;
                    {
                        const auto& pts = reference_curve_points_;
                        if (new_x <= pts.front().x) ref_y = pts.front().y;
                        else if (new_x >= pts.back().x) ref_y = pts.back().y;
                        else {
                            for (size_t j = 0; j < pts.size() - 1; ++j) {
                                if (new_x >= pts[j].x && new_x <= pts[j + 1].x) {
                                    double t = (new_x - pts[j].x) / (pts[j + 1].x - pts[j].x);
                                    ref_y = pts[j].y + t * (pts[j + 1].y - pts[j].y);
                                    break;
                                }
                            }
                        }
                    }
                    const double snap_y_threshold = 0.01; // 1% output snap
                    if (std::fabs(new_y - ref_y) < snap_y_threshold) {
                        new_y = ref_y;
                    }

                    // Optional X snap to nearest reference control point
                    double closest_dx = 1.0;
                    double closest_x = new_x;
                    for (const auto& rp : reference_curve_points_) {
                        double dx = std::fabs(new_x - rp.x);
                        if (dx < closest_dx) {
                            closest_dx = dx;
                            closest_x = rp.x;
                        }
                    }
                    const double snap_x_threshold = 0.01;
                    if (closest_dx < snap_x_threshold && !is_first_point && !is_last_point) {
                        new_x = closest_x;
                    }
                }

                // Constrain Y to Windows API valid zone (50-150% of identity)
                double min_y = new_x * 0.5;
                double max_y = (std::min)(1.0, new_x * 1.5 + 0.05);
                new_y = std::clamp(new_y, min_y, max_y);

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

                    // Constrain Y to Windows API valid zone (50-150% of identity)
                    double min_y = new_x * 0.5;
                    double max_y = (std::min)(1.0, new_x * 1.5 + 0.05);
                    new_y = std::clamp(new_y, min_y, max_y);

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
}

void MainWindow::renderHelpTab()
{
    ImGui::Spacing();

    ImGui::TextUnformatted("What This Tool Does");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextWrapped(
        "Lumos applies a global GPU output remap (1D LUT) to all displays. "
        "This is a quick visibility tweak for dark scenes, NOT color calibration.");

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.6f, 1.0f), "Important:");
    ImGui::BulletText("Affects the entire desktop and all applications");
    ImGui::BulletText("Applied after Windows color management");
    ImGui::BulletText("No ICC profiles or measurements involved");
    ImGui::BulletText("Use Reset to restore captured defaults");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextUnformatted("Tone Curve Presets");
    ImGui::Spacing();
    ImGui::BulletText("Neutral: Identity curve (no change)");
    ImGui::BulletText("Simple Gamma: Traditional power-law curve");
    ImGui::BulletText("Shadow Lift: Raises dark values for visibility");
    ImGui::BulletText("Soft Contrast: Gentle S-curve");
    ImGui::BulletText("Cinema: Aggressive gamma 2.6 curve");
    ImGui::BulletText("Custom: Edit your own curve");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextUnformatted("Global Hotkeys");
    ImGui::Spacing();
    ImGui::TextDisabled("(Customize via Edit > Settings)");
    ImGui::Spacing();

    ImGui::Columns(2, "hotkeys", false);
    ImGui::SetColumnWidth(0, 150);
    ImGui::Text("Increase"); ImGui::NextColumn();
    ImGui::Text("Increase curve strength (default: Ctrl+Alt+Up)"); ImGui::NextColumn();
    ImGui::Text("Decrease"); ImGui::NextColumn();
    ImGui::Text("Decrease curve strength (default: Ctrl+Alt+Down)"); ImGui::NextColumn();
    ImGui::Text("Reset"); ImGui::NextColumn();
    ImGui::Text("Restore captured defaults (default: Ctrl+Alt+R)"); ImGui::NextColumn();
    ImGui::Text("Toggle"); ImGui::NextColumn();
    ImGui::Text("Turn gamma on/off (default: Ctrl+Alt+G)"); ImGui::NextColumn();
    ImGui::Columns(1);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextUnformatted("Command Line");
    ImGui::Spacing();
    ImGui::TextDisabled("lumos              Open the GUI");
    ImGui::TextDisabled("lumos 1.2          Set curve strength and exit");
    ImGui::TextDisabled("lumos --help       Show help");
    ImGui::TextDisabled("lumos --version    Show version");
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

void MainWindow::renderSettingsTab(App& app)
{
    ImGui::Spacing();

    // Initialize edit state from app on first render or when tab reopens
    if (!hotkey_settings_initialized_) {
        edit_hotkey_increase_ = app.getHotkeyIncrease();
        edit_hotkey_decrease_ = app.getHotkeyDecrease();
        edit_hotkey_reset_ = app.getHotkeyReset();
        edit_hotkey_toggle_ = app.getHotkeyToggle();
        hotkey_settings_initialized_ = true;
        hotkey_settings_dirty_ = false;
    }

    ImGui::TextUnformatted("Global Hotkeys");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextWrapped(
        "Configure keyboard shortcuts for gamma adjustments. "
        "Hotkeys work system-wide, even when Lumos is minimized.");

    ImGui::Spacing();

    // Get bindable keys for dropdown
    const auto& keys = HotkeyUtils::getBindableKeys();

    // Helper to find key index
    auto findKeyIndex = [&keys](UINT vk) -> int {
        for (size_t i = 0; i < keys.size(); ++i) {
            if (keys[i].vk == vk) return static_cast<int>(i);
        }
        return 0;
    };

    // Helper to render a hotkey row
    auto renderHotkeyRow = [&](const char* label, HotkeyBinding& binding) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(label);

        ImGui::TableNextColumn();
        // Display current binding
        std::string display = HotkeyUtils::bindingToString(binding);
        ImGui::TextDisabled("%s", display.c_str());

        ImGui::TableNextColumn();
        // Modifier checkboxes
        bool ctrl = (binding.modifiers & MOD_CONTROL) != 0;
        bool alt = (binding.modifiers & MOD_ALT) != 0;
        bool shift = (binding.modifiers & MOD_SHIFT) != 0;

        ImGui::PushID(label);
        if (ImGui::Checkbox("Ctrl", &ctrl)) {
            binding.modifiers = (binding.modifiers & ~MOD_CONTROL) | (ctrl ? MOD_CONTROL : 0);
            hotkey_settings_dirty_ = true;
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Alt", &alt)) {
            binding.modifiers = (binding.modifiers & ~MOD_ALT) | (alt ? MOD_ALT : 0);
            hotkey_settings_dirty_ = true;
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Shift", &shift)) {
            binding.modifiers = (binding.modifiers & ~MOD_SHIFT) | (shift ? MOD_SHIFT : 0);
            hotkey_settings_dirty_ = true;
        }

        ImGui::TableNextColumn();
        // Key dropdown
        int current_idx = findKeyIndex(binding.key);
        ImGui::SetNextItemWidth(100);
        if (ImGui::BeginCombo("##key", keys[current_idx].name)) {
            for (size_t i = 0; i < keys.size(); ++i) {
                bool selected = (static_cast<int>(i) == current_idx);
                if (ImGui::Selectable(keys[i].name, selected)) {
                    binding.key = keys[i].vk;
                    hotkey_settings_dirty_ = true;
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopID();
    };

    // Hotkey table
    if (ImGui::BeginTable("HotkeyTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 120);
        ImGui::TableSetupColumn("Current", ImGuiTableColumnFlags_WidthFixed, 130);
        ImGui::TableSetupColumn("Modifiers", ImGuiTableColumnFlags_WidthFixed, 180);
        ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        renderHotkeyRow("Increase Gamma", edit_hotkey_increase_);
        renderHotkeyRow("Decrease Gamma", edit_hotkey_decrease_);
        renderHotkeyRow("Reset Gamma", edit_hotkey_reset_);
        renderHotkeyRow("Toggle On/Off", edit_hotkey_toggle_);

        ImGui::EndTable();
    }

    // Error display
    const char* error = app.getHotkeyError();
    if (error && error[0] != '\0') {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", error);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Apply/Revert buttons
    if (hotkey_settings_dirty_) {
        if (ImGui::Button("Apply Changes")) {
            if (app.setHotkeys(edit_hotkey_increase_,
                               edit_hotkey_decrease_,
                               edit_hotkey_reset_,
                               edit_hotkey_toggle_)) {
                hotkey_settings_dirty_ = false;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Revert")) {
            edit_hotkey_increase_ = app.getHotkeyIncrease();
            edit_hotkey_decrease_ = app.getHotkeyDecrease();
            edit_hotkey_reset_ = app.getHotkeyReset();
            edit_hotkey_toggle_ = app.getHotkeyToggle();
            hotkey_settings_dirty_ = false;
        }
    } else {
        ImGui::BeginDisabled();
        ImGui::Button("Apply Changes");
        ImGui::SameLine();
        ImGui::Button("Revert");
        ImGui::EndDisabled();
    }

    // Hint text
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::TextDisabled("Note: Hotkeys must have at least one modifier (Ctrl, Alt, or Shift).");
    ImGui::TextDisabled("Some key combinations may be reserved by Windows or other applications.");

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextUnformatted("Display Options");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Checkbox("Show Histogram", &show_histogram_);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Shows screen luminance distribution in the curve preview");
    }

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextUnformatted("Window Behavior");
    ImGui::Separator();
    ImGui::Spacing();

    bool minimize_to_tray = app.getMinimizeToTrayOnClose();
    if (ImGui::Checkbox("Minimize to system tray when closed", &minimize_to_tray)) {
        app.setMinimizeToTrayOnClose(minimize_to_tray);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("When enabled, closing the window minimizes to tray instead of exiting");
    }

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextUnformatted("Reset");
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Reset Gamma", ImVec2(-1, 0))) {
        app.resetGamma();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Restores the GPU output curve captured at startup");
    }
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
