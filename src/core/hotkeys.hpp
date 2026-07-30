#pragma once

namespace got {

// Polls the configurable global hotkeys (see TrainerHotkeys) once per
// frame and toggles the matching settings on key-down edges, pushing a
// toast notification for feedback. The menu-open hotkey itself is handled
// separately in the WndProc hook so it still works while the overlay is
// hidden; this covers the always-on feature hotkeys (Infinite Health,
// Stealth, Free Cam, Panic).
void ProcessHotkeys();

}  // namespace got
