# Ghost of Tsushima Trainer

A native C++ trainer for **Ghost of Tsushima (PC)**, loaded via a `version.dll` proxy and rendered with a Dear ImGui overlay. Built for single-player use — save-editing style conveniences and fun toggles for your own playthrough, photo mode runs, and screenshots.

> Game version tested: `v1053.5.0625.1621`

## Features

**Player**
- Infinite Health
- Infinite Resolve
- One-click Fill Health / Fill Resolve
- Save & teleport to a saved position (`F1` / `F2`)

**Combat**
- Stealth (enemies can't detect you)
- No Detection Indicator
- Force Perfect Dodge
- Auto Block Melee
- Always Parry on Block / Perfect Parry

**Movement & Camera**
- Movement Speed multiplier (adjustable)
- Free Camera with WASD/IJKL + Shift boost
- Teleport player to camera position (`F3`)

**Items**
- Add any resource (arrows, hides, ore, gold, etc.) with a custom amount
- Give gear by hash

**Quality of life**
- Fully rebindable hotkeys (click-to-bind, no config file editing required)
- On-screen toast notifications for every toggle and action
- Settings persist automatically between sessions (`got_trainer.ini`)
- One-button **Panic switch** — instantly disables every feature and restores all patched memory, useful before alt-tabbing or recording
- Clean, self-contained dark UI — no debug/internal information exposed

## Installation

1. Download the release archive and extract `version.dll` into your Ghost of Tsushima install folder (next to `GhostOfTsushima.exe`).
2. Launch the game normally through Steam/Epic.
3. Once you're in-game, press **F8** to open the trainer menu (rebindable in the Hotkeys tab).

> If you already run another mod that also ships a `version.dll` proxy, they may conflict — only one proxy DLL can occupy that filename per game folder.

## Building from source

Requirements: CMake 3.20+, a C++20-capable MinGW-w64 or MSVC toolchain, Windows.

```bash
cmake -B build -G "MinGW Makefiles"
cmake --build build
```

The resulting `version.dll` will be in `build/bin/`. Dependencies (Dear ImGui, MinHook) are fetched automatically via CMake `FetchContent`.

## Project layout

```
src/
  core/       process hooking, pattern scanning, logging, config, hotkeys, notifications
  features/   individual feature modules (player, combat, movement, camera, items)
  proxy/      version.dll export forwarding
  render/     DirectX 11/12 present hook + ImGui backend wiring
  ui/         trainer overlay (theme + tabs)
```

## Safety notes

- This trainer works entirely in your own game process for single-player use. It does not modify game files on disk and does not touch anything network-related.
- Ghost of Tsushima's story mode has no anti-cheat or online components affected by this tool, but as with any memory trainer, disable it before touching Trophies/Achievements runs you want to keep "clean", and don't use it in any online-adjacent context.
- Use the Panic switch (default **F9**) any time you want everything instantly reverted to stock behavior.

## Known limitations

- **Pickup Multiplier** and **Unlock All Gear on Save Load** are present in the UI but disabled — they need a fresh pattern signature after the last game update. Tracked for a follow-up release.
- Free Camera and some combat patches rely on AOB signatures that may need re-scanning (`Status` tab → **Rescan**) after a game patch changes code layout.

## Credits

Built with [Dear ImGui](https://github.com/ocornut/imgui) and [MinHook](https://github.com/TsudaKageyu/minhook).

## Disclaimer

Provided as-is, for single-player/offline use only. Not affiliated with Sucker Punch Productions or Sony Interactive Entertainment.
