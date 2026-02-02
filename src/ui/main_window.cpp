// Lumos - Main UI window
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <d3d11.h>

#include "main_window.h"
#include "../app.h"
#include "../../resources/resource.h"
#include "imgui.h"
#include "implot.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace lumos::ui {

MainWindow::~MainWindow()
{
    if (bolt_texture_) {
        bolt_texture_->Release();
        bolt_texture_ = nullptr;
    }
    if (bolt_slash_texture_) {
        bolt_slash_texture_->Release();
        bolt_slash_texture_ = nullptr;
    }
}

// Helper to create a D3D11 texture from an HICON
static ID3D11ShaderResourceView* CreateTextureFromIcon(ID3D11Device* device, HICON hIcon, int size)
{
    if (!hIcon || !device) return nullptr;

    // Get icon info
    ICONINFO iconInfo = {};
    if (!GetIconInfo(hIcon, &iconInfo)) return nullptr;

    // Get bitmap info
    BITMAP bm = {};
    GetObject(iconInfo.hbmColor ? iconInfo.hbmColor : iconInfo.hbmMask, sizeof(bm), &bm);

    int width = bm.bmWidth;
    int height = bm.bmHeight;
    if (width <= 0 || height <= 0) {
        if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);
        if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);
        return nullptr;
    }

    // Create a compatible DC and bitmap to extract pixels
    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = width;
    bi.bmiHeader.biHeight = -height; // Top-down
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP hBitmap = CreateDIBSection(hdcMem, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);

    if (hBitmap && bits) {
        HGDIOBJ oldBitmap = SelectObject(hdcMem, hBitmap);
        DrawIconEx(hdcMem, 0, 0, hIcon, width, height, 0, nullptr, DI_NORMAL);
        SelectObject(hdcMem, oldBitmap);

        // Convert BGRA to RGBA and invert colors for dark mode
        unsigned char* pixels = static_cast<unsigned char*>(bits);
        for (int i = 0; i < width * height; ++i) {
            unsigned char b = pixels[i * 4 + 0];
            unsigned char g = pixels[i * 4 + 1];
            unsigned char r = pixels[i * 4 + 2];
            unsigned char a = pixels[i * 4 + 3];

            // Invert RGB for dark mode (black icon -> white icon)
            pixels[i * 4 + 0] = 255 - r;  // R
            pixels[i * 4 + 1] = 255 - g;  // G
            pixels[i * 4 + 2] = 255 - b;  // B
            pixels[i * 4 + 3] = a;        // A
        }

        // Create D3D11 texture
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = pixels;
        initData.SysMemPitch = width * 4;

        ID3D11Texture2D* texture = nullptr;
        HRESULT hr = device->CreateTexture2D(&desc, &initData, &texture);

        ID3D11ShaderResourceView* srv = nullptr;
        if (SUCCEEDED(hr) && texture) {
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = desc.Format;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;

            device->CreateShaderResourceView(texture, &srvDesc, &srv);
            texture->Release();
        }

        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(nullptr, hdcScreen);

        if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);
        if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);

        return srv;
    }

    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);
    if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);
    if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);

    return nullptr;
}

void MainWindow::initIconTextures(ID3D11Device* device)
{
    HINSTANCE hInstance = GetModuleHandleW(nullptr);

    // Load 16x16 icons
    HICON hBolt = static_cast<HICON>(LoadImageW(
        hInstance, MAKEINTRESOURCEW(IDI_BOLT), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
    HICON hBoltSlash = static_cast<HICON>(LoadImageW(
        hInstance, MAKEINTRESOURCEW(IDI_BOLT_SLASH), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));

    if (hBolt) {
        bolt_texture_ = CreateTextureFromIcon(device, hBolt, 16);
        DestroyIcon(hBolt);
    }
    if (hBoltSlash) {
        bolt_slash_texture_ = CreateTextureFromIcon(device, hBoltSlash, 16);
        DestroyIcon(hBoltSlash);
    }
}

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

    // Power icon on the right (clickable to toggle gamma)
    bool gamma_active = app.isGammaEnabled();
    float icon_x = window_pos.x + window_size.x - padding_x - icon_size;

    // Draw vertical separator before icon
    float separator_x = icon_x - 8.0f;
    draw_list->AddLine(
        ImVec2(separator_x, status_y + 4.0f),
        ImVec2(separator_x, status_y + status_height - 4.0f),
        IM_COL32(60, 60, 60, 255), 1.0f);

    // Draw power icon using texture
    ImVec2 icon_pos(icon_x, content_y);
    ID3D11ShaderResourceView* icon_texture = gamma_active ? bolt_texture_ : bolt_slash_texture_;
    if (icon_texture) {
        // Tint: green when active, gray when inactive
        ImU32 tint = gamma_active ? IM_COL32(100, 200, 120, 255) : IM_COL32(150, 150, 150, 255);
        draw_list->AddImage(
            reinterpret_cast<ImTextureID>(icon_texture),
            icon_pos,
            ImVec2(icon_pos.x + icon_size, icon_pos.y + icon_size),
            ImVec2(0, 0), ImVec2(1, 1),
            tint);
    }

    // Set cursor position for invisible button (clickable + tooltip)
    ImGui::SetCursorScreenPos(icon_pos);
    ImGui::InvisibleButton("##PowerToggle", ImVec2(icon_size, icon_size));

    if (ImGui::IsItemClicked()) {
        app.toggleGamma();
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(gamma_active ? "Click to disable gamma" : "Click to enable gamma");
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

    // Tone curve visualization using ImPlot
    ImGui::Spacing();
    ImGui::TextUnformatted("Output Curve Preview");
    ImGui::TextDisabled("Scroll to zoom | Drag to pan | Double-click to reset view");

    // Initialize plot limits on first frame
    if (!plot_limits_initialized_) {
        plot_x_min_ = 0.0;
        plot_x_max_ = 1.0;
        plot_y_min_ = 0.0;
        plot_y_max_ = 1.0;
        plot_limits_initialized_ = true;
    }

    // Reset zoom button
    if (ImGui::Button("Reset Zoom")) {
        plot_x_min_ = 0.0;
        plot_x_max_ = 1.0;
        plot_y_min_ = 0.0;
        plot_y_max_ = 1.0;
    }
    ImGui::SameLine();
    if (ImGui::Button("Fit to Curve")) {
        // Auto-fit to curve extents with some padding
        plot_x_min_ = -0.05;
        plot_x_max_ = 1.05;
        plot_y_min_ = -0.05;
        plot_y_max_ = 1.05;
    }

    // Sort curve points before any operations
    if (is_custom_mode && !ui_curve_points_.empty()) {
        std::sort(ui_curve_points_.begin(), ui_curve_points_.end());
    }
    if (is_custom_mode && !reference_curve_points_.empty()) {
        std::sort(reference_curve_points_.begin(), reference_curve_points_.end());
    }

    // Prepare curve data for ImPlot (256 samples)
    static std::vector<double> curve_xs(256);
    static std::vector<double> curve_ys(256);
    static std::vector<double> linear_xs(2);
    static std::vector<double> linear_ys(2);
    static std::vector<double> ref_curve_xs(256);
    static std::vector<double> ref_curve_ys(256);

    // Linear reference line
    linear_xs[0] = 0.0; linear_ys[0] = 0.0;
    linear_xs[1] = 1.0; linear_ys[1] = 1.0;

    const double gamma = static_cast<double>(gamma_slider_);

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
                return std::pow(linear, 1.0 / gamma);
        }
    };

    // Generate curve data
    for (int i = 0; i < 256; ++i) {
        double x = i / 255.0;
        curve_xs[i] = x;
        curve_ys[i] = eval_curve(x);
    }

    // Generate reference curve data for custom mode
    if (is_custom_mode && !reference_curve_points_.empty()) {
        auto eval_reference = [&](double linear) -> double {
            const auto& pts = reference_curve_points_;
            if (linear <= pts.front().x) return pts.front().y;
            if (linear >= pts.back().x) return pts.back().y;
            for (size_t j = 0; j < pts.size() - 1; ++j) {
                if (linear >= pts[j].x && linear <= pts[j + 1].x) {
                    double t = (linear - pts[j].x) / (pts[j + 1].x - pts[j].x);
                    return pts[j].y + t * (pts[j + 1].y - pts[j].y);
                }
            }
            return linear;
        };
        for (int i = 0; i < 256; ++i) {
            double x = i / 255.0;
            ref_curve_xs[i] = x;
            ref_curve_ys[i] = eval_reference(x);
        }
    }

    // Prepare histogram data if enabled
    static std::vector<double> hist_xs(256);
    static std::vector<double> hist_ys_pre(256);
    static std::vector<double> hist_ys_post(256);

    if (show_histogram_) {
        auto histogram = app.getScreenHistogram();
        if (histogram.valid) {
            // Reset arrays
            for (int i = 0; i < 256; ++i) {
                hist_xs[i] = i / 255.0;
                histogram_ys_[i] = histogram.luminance[i];
                histogram_ys_post_[i] = 0.0f;
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

            // Copy to double arrays for ImPlot
            for (int i = 0; i < 256; ++i) {
                hist_ys_pre[i] = histogram_ys_[i] * 0.7;  // Scale to 70% height
                hist_ys_post[i] = histogram_ys_post_[i] * 0.7;
            }
        }
    }

    // Initialize ui_curve_points if in custom mode and not yet initialized
    if (is_custom_mode && ui_curve_points_.empty()) {
        ui_curve_points_ = app.getCustomCurvePoints();
    }

    // Prepare valid zone data for custom mode
    static std::vector<double> valid_zone_xs(65);
    static std::vector<double> valid_zone_min(65);
    static std::vector<double> valid_zone_max(65);
    if (is_custom_mode) {
        for (int i = 0; i <= 64; ++i) {
            double x_norm = i / 64.0;
            valid_zone_xs[i] = x_norm;
            valid_zone_min[i] = 0.0;
            valid_zone_max[i] = (std::min)(1.0, x_norm * 3.0 + 0.2);
        }
    }

    // ImPlot curve editor with zoom/pan
    ImPlotFlags plot_flags = ImPlotFlags_NoTitle | ImPlotFlags_NoLegend;
    ImPlotAxisFlags axis_flags = ImPlotAxisFlags_None;

    // Use linked axis limits for persistent zoom state
    ImPlot::SetNextAxesLimits(plot_x_min_, plot_x_max_, plot_y_min_, plot_y_max_, ImPlotCond_Once);

    if (ImPlot::BeginPlot("##CurveEditor", ImVec2(-1, 250), plot_flags)) {
        // Setup axes with linked limits for zoom persistence
        ImPlot::SetupAxis(ImAxis_X1, "Input", axis_flags);
        ImPlot::SetupAxis(ImAxis_Y1, "Output", axis_flags);
        ImPlot::SetupAxisLinks(ImAxis_X1, &plot_x_min_, &plot_x_max_);
        ImPlot::SetupAxisLinks(ImAxis_Y1, &plot_y_min_, &plot_y_max_);

        // Set axis zoom constraints (don't allow zooming too far out)
        ImPlot::SetupAxisZoomConstraints(ImAxis_X1, 0.01, 2.0);
        ImPlot::SetupAxisZoomConstraints(ImAxis_Y1, 0.01, 2.0);

        // Draw histogram as bars (if enabled and valid)
        if (show_histogram_) {
            auto histogram = app.getScreenHistogram();
            if (histogram.valid) {
                ImPlot::SetNextFillStyle(ImVec4(0.24f, 0.35f, 0.59f, 0.4f));
                ImPlot::PlotBars("Pre", hist_xs.data(), hist_ys_pre.data(), 256, 0.003);
                ImPlot::SetNextFillStyle(ImVec4(0.78f, 0.55f, 0.24f, 0.4f));
                ImPlot::PlotBars("Post", hist_xs.data(), hist_ys_post.data(), 256, 0.003);
            }
        }

        // Draw valid zone (shaded area) for custom mode
        if (is_custom_mode) {
            ImPlot::SetNextFillStyle(ImVec4(0.2f, 0.4f, 0.2f, 0.3f));
            ImPlot::PlotShaded("ValidZone", valid_zone_xs.data(), valid_zone_min.data(), valid_zone_max.data(), 65);
        }

        // Draw linear reference line (identity)
        ImPlot::SetNextLineStyle(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), 1.5f);
        ImPlot::PlotLine("Identity", linear_xs.data(), linear_ys.data(), 2);

        // Draw reference curve (dashed) for custom mode
        if (is_custom_mode && !reference_curve_points_.empty()) {
            ImPlot::SetNextLineStyle(ImVec4(0.7f, 0.7f, 0.7f, 0.6f), 1.0f);
            ImPlot::PlotLine("Reference", ref_curve_xs.data(), ref_curve_ys.data(), 256);
        }

        // Draw main tone curve
        ImPlot::SetNextLineStyle(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), 2.5f);
        ImPlot::PlotLine("Curve", curve_xs.data(), curve_ys.data(), 256);

        // Interactive control points using ImPlot::DragPoint (only in Custom mode)
        if (is_custom_mode && !ui_curve_points_.empty()) {
            bool curve_modified = false;

            for (size_t i = 0; i < ui_curve_points_.size(); ++i) {
                // Store points in temporary variables for DragPoint
                double px = ui_curve_points_[i].x;
                double py = ui_curve_points_[i].y;

                // Determine drag flags - lock X for first and last points
                ImPlotDragToolFlags drag_flags = ImPlotDragToolFlags_None;
                bool is_first = (i == 0);
                bool is_last = (i == ui_curve_points_.size() - 1);

                // Different colors for different states
                ImVec4 point_color = ImVec4(0.4f, 0.8f, 0.4f, 1.0f);  // Green
                if (static_cast<int>(i) == selected_point_index_) {
                    point_color = ImVec4(1.0f, 0.8f, 0.4f, 1.0f);  // Orange when selected
                }

                char point_id[32];
                std::snprintf(point_id, sizeof(point_id), "##pt%zu", i);

                bool clicked = false;
                bool hovered = false;
                bool held = false;

                if (ImPlot::DragPoint(static_cast<int>(i), &px, &py, point_color, 8.0f, drag_flags, &clicked, &hovered, &held)) {
                    // Point was dragged
                    curve_modified = true;

                    // Lock X for endpoints
                    if (is_first) px = 0.0;
                    if (is_last) px = 1.0;

                    // Clamp to valid range
                    px = std::clamp(px, 0.0, 1.0);

                    // Constrain Y to valid zone
                    double min_y = 0.0;
                    double max_y = (std::min)(1.0, px * 3.0 + 0.2);
                    py = std::clamp(py, min_y, max_y);

                    // Snap to reference curve if close
                    if (!reference_curve_points_.empty()) {
                        // Interpolate reference Y at this X
                        double ref_y = py;
                        const auto& pts = reference_curve_points_;
                        if (px <= pts.front().x) ref_y = pts.front().y;
                        else if (px >= pts.back().x) ref_y = pts.back().y;
                        else {
                            for (size_t j = 0; j < pts.size() - 1; ++j) {
                                if (px >= pts[j].x && px <= pts[j + 1].x) {
                                    double t = (px - pts[j].x) / (pts[j + 1].x - pts[j].x);
                                    ref_y = pts[j].y + t * (pts[j + 1].y - pts[j].y);
                                    break;
                                }
                            }
                        }
                        const double snap_threshold = 0.02;
                        if (std::fabs(py - ref_y) < snap_threshold) {
                            py = ref_y;
                        }
                    }

                    ui_curve_points_[i].x = px;
                    ui_curve_points_[i].y = py;
                }

                if (held) {
                    selected_point_index_ = static_cast<int>(i);
                }

                // Show tooltip on hover
                if (hovered) {
                    ImGui::BeginTooltip();
                    ImGui::Text("X: %.3f, Y: %.3f", px, py);
                    if (is_first) ImGui::TextDisabled("(First point - X locked)");
                    if (is_last) ImGui::TextDisabled("(Last point - X locked)");
                    ImGui::EndTooltip();
                }
            }

            // Apply changes if modified
            if (curve_modified) {
                std::sort(ui_curve_points_.begin(), ui_curve_points_.end());
                app.setCustomCurvePoints(ui_curve_points_);
            }

            // Handle adding new points with Ctrl+Click
            if (ImPlot::IsPlotHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::GetIO().KeyCtrl) {
                ImPlotPoint mouse = ImPlot::GetPlotMousePos();
                double new_x = std::clamp(mouse.x, 0.0, 1.0);
                double new_y = std::clamp(mouse.y, 0.0, 1.0);

                // Constrain Y to valid zone
                double min_y = 0.0;
                double max_y = (std::min)(1.0, new_x * 3.0 + 0.2);
                new_y = std::clamp(new_y, min_y, max_y);

                ui_curve_points_.push_back({new_x, new_y});
                std::sort(ui_curve_points_.begin(), ui_curve_points_.end());
                app.setCustomCurvePoints(ui_curve_points_);
            }

            // Handle deleting points with right-click
            if (ImPlot::IsPlotHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                ImPlotPoint mouse = ImPlot::GetPlotMousePos();

                // Find closest point
                int closest = -1;
                double closest_dist = 0.05;  // Threshold in plot units
                for (size_t i = 0; i < ui_curve_points_.size(); ++i) {
                    double dx = ui_curve_points_[i].x - mouse.x;
                    double dy = ui_curve_points_[i].y - mouse.y;
                    double dist = std::sqrt(dx * dx + dy * dy);
                    if (dist < closest_dist) {
                        closest_dist = dist;
                        closest = static_cast<int>(i);
                    }
                }

                // Delete if found and not an endpoint
                if (closest > 0 && closest < static_cast<int>(ui_curve_points_.size()) - 1 && ui_curve_points_.size() > 2) {
                    ui_curve_points_.erase(ui_curve_points_.begin() + closest);
                    app.setCustomCurvePoints(ui_curve_points_);
                    selected_point_index_ = -1;
                }
            }
        }

        // Show mouse position in plot coordinates
        if (ImPlot::IsPlotHovered()) {
            ImPlotPoint mouse = ImPlot::GetPlotMousePos();
            ImGui::BeginTooltip();
            ImGui::Text("(%.3f, %.3f)", mouse.x, mouse.y);
            if (is_custom_mode) {
                ImGui::TextDisabled("Ctrl+Click: Add point");
                ImGui::TextDisabled("Right-click: Delete point");
            }
            ImGui::EndTooltip();
        }

        ImPlot::EndPlot();
    }
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

    bool always_on_top = app.getAlwaysOnTop();
    if (ImGui::Checkbox("Always on top", &always_on_top)) {
        app.setAlwaysOnTop(always_on_top);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Keep the Lumos window above all other windows");
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
