#pragma once

namespace got {

// Loads persisted trainer settings + hotkeys from "got_trainer.ini" next to
// the game executable. Safe no-op if the file does not exist yet.
void LoadConfig();

// Writes the current trainer settings + hotkeys to "got_trainer.ini".
// Called automatically whenever the menu is closed and on unload, and can
// also be triggered manually from the Settings tab.
void SaveConfig();

}  // namespace got
