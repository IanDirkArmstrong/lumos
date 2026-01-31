// Lumos - Main UI window
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#include "main_window.h"
#include "../app.h"
#include "imgui.h"

namespace lumos::ui {

void MainWindow::render(App& app)
{
    // Sync slider with app state on first frame
    if (first_frame_) {
        gamma_slider_ = static_cast<float>(app.getGamma());
        first_frame_ = false;
    }

    // Full window ImGui panel
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("##MainWindow", nullptr, flags);

    // Title
    ImGui::TextUnformatted("Lumos - Gamma Control");
    ImGui::Separator();
    ImGui::Spacing();

    // Gamma slider
    ImGui::TextUnformatted("Gamma");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##Gamma", &gamma_slider_, 0.1f, 9.0f, "%.2f")) {
        app.setGamma(static_cast<double>(gamma_slider_));
    }

    ImGui::Spacing();

    // Current value display
    ImGui::Text("Current: %.2f", gamma_slider_);

    ImGui::Spacing();
    ImGui::Spacing();

    // Reset button
    if (ImGui::Button("Reset to Default", ImVec2(-1, 0))) {
        app.resetGamma();
        gamma_slider_ = static_cast<float>(app.getGamma());
    }

    // Status at bottom
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("%s", app.getStatusText());

    ImGui::End();
}

} // namespace lumos::ui
