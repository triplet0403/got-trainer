#pragma once

namespace got {

// Applies the trainer's custom ImGui theme (dark background, teal/gold
// accent, rounded widgets). Call once, right after ImGui::StyleColorsDark(),
// during backend init (both the DX11 and DX12 init paths call this).
void ApplyTrainerTheme();

// Draws the full trainer overlay (tab bar + all tabs). Call once per
// frame, after ImGui::NewFrame(), only while the menu is open.
void RenderTrainerUI();

}  // namespace got
