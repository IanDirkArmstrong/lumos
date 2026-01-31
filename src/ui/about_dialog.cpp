// Lumos - About dialog
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#include "about_dialog.h"
#include "imgui.h"

namespace lumos::ui {

void AboutDialog::render(bool* p_open)
{
    if (!visible_) return;

    ImGui::SetNextWindowSize(ImVec2(350, 280), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("About Lumos", &visible_, ImGuiWindowFlags_NoResize)) {
        ImGui::TextUnformatted("Lumos v0.2.0");
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

        ImGui::Spacing();
        ImGui::Spacing();

        if (ImGui::Button("Close", ImVec2(-1, 0))) {
            visible_ = false;
        }
    }
    ImGui::End();

    if (p_open) *p_open = visible_;
}

} // namespace lumos::ui
