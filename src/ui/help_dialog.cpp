// Lumos - Help dialog
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#include "help_dialog.h"
#include "imgui.h"

namespace lumos::ui {

void HelpDialog::render(bool* p_open)
{
    if (!visible_) return;

    ImGui::SetNextWindowSize(ImVec2(400, 350), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Lumos Help", &visible_, ImGuiWindowFlags_NoResize)) {
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
        ImGui::Spacing();

        if (ImGui::Button("Close", ImVec2(-1, 0))) {
            visible_ = false;
        }
    }
    ImGui::End();

    if (p_open) *p_open = visible_;
}

} // namespace lumos::ui
