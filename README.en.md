# REC HUD - recording overlay for OBS Studio

[English](README.en.md) | [Italiano](README.md)

Native OBS plugin that shows **an "REC" indicator always on top of your desktop**, with the **recording duration** (toggleable).

Built for the case where you are recording a game in **fullscreen** and OBS is hidden: the overlay stays visible to you, above everything else, and never ends up in the recording.

## Features

- Pulsing red dot + `REC` label while recording
- **Duration** `MM:SS` (or `HH:MM:SS`), toggleable in the settings
- `PAUSED` indicator while paused (the timer **freezes** and resumes where it left off)
- Frameless, transparent, always-on-top window
- **Click-through**: the mouse passes through, so it never interferes with your game
- Drag it anywhere, or dock it to a screen corner
- Customisable colours, text size, corner radius, margin
- Settings saved per **OBS profile**
- UI and menus translated in English and Italian

## Requirements

- Windows 10/11 x64
- [Visual Studio](https://visualstudio.microsoft.com/) 2019/2022/2026 -- pick the **"Desktop development with C++"** workload during installation
- [CMake](https://cmake.org/download/) 3.28+ (`winget install Kitware.CMake`)
- **Git is not required**: the scripts fetch the official template as a zip archive

On first run CMake downloads the dependencies automatically (libobs, obs-frontend-api, Qt6): roughly 1 GB and a few minutes.

## Building

```powershell
# 1. Prepare the workspace (downloads obs-plugintemplate and applies our sources)
.\scripts\setup.ps1

# 2. Build (auto-detects the Visual Studio generator and the cmake install path)
.\scripts\build.ps1

# 3. Install
.\scripts\install.ps1
```

### If PowerShell blocks the scripts

If `.\scripts\setup.ps1` fails with *"Running scripts is disabled on this system"*:

```powershell
Set-ExecutionPolicy -Scope CurrentUser RemoteSigned
```

Or, without changing any setting, run each time with:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\setup.ps1
```

### Where it gets installed

By default into `C:\ProgramData\obs-studio\plugins\obs-rec-hud\`, which **does not require administrator rights**:

```
bin\64bit\obs-rec-hud.dll
data\locale\en-US.ini
data\locale\it-IT.ini
```

To install into the OBS install folder instead (requires an elevated PowerShell):

```powershell
.\scripts\install.ps1 -Location InstallDir
```

## Usage

1. Start OBS Studio
2. **Tools -> REC HUD settings...**
3. **General** tab:
   - **Show the recording duration** -> turn the timer on/off
   - **Keep the overlay above every other window** -> stays on top of the game
   - **Click-through** -> the mouse passes through the overlay
   - **Always visible** -> handy while positioning it
4. **Position** tab -> pick a corner (default: top right)
5. Start recording: the overlay appears and the timer starts at `00:00`

To move it by hand: turn off **Click-through**, start a recording and drag the overlay. The position is saved in the profile. To go back to automatic docking use **Tools > REC HUD: reset position** (or pick a corner in the *Position* tab).

## Diagnostics

The plugin writes a few lines to the OBS log
(`%APPDATA%\obs-studio\logs\*.txt`):

```
[rec-hud] loaded
[rec-hud] tools menu entries added
[rec-hud] recording started, overlay shown
[rec-hud] recording stopped
```

If the overlay does not show up, check that `Enabled` is ticked in the settings and that no saved manual position is pushing it off screen: use **reset position**.

## Technical notes

- The overlay is a Qt window (`Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint`) created by the plugin itself: it is a genuine OBS integration (it starts with OBS and goes away when OBS closes).
- Recording state comes from the frontend events `OBS_FRONTEND_EVENT_RECORDING_STARTED/STOPPED/PAUSED/UNPAUSED`, plus a 1 s polling timer as a safety net (hotkeys, profile switches, plugin reloads).
- Corner docking is **deferred by one tick**: resizing and repositioning in the same cycle makes Windows apply the resize *after* the move, which pushes the window off screen whenever the label grows.
- The duration uses an accumulator: `QElapsedTimer::restart()` resets the counter, so the time elapsed during pauses is summed separately.
- It is **never captured** into the recording, unless you explicitly capture that window (Window Capture / "Capture Window").
- Settings are per-profile: every OBS profile keeps its own configuration.

## Project layout

```
CMakeLists.txt          build (derived from obs-plugintemplate)
buildspec.json          name, version and dependencies
CMakePresets.json       presets for the IDE / CMake GUI
src/
  plugin-main.cpp       module entry point
  rec-hud.{h,cpp}       controller: OBS events, polling, persistence
  rec-hud-config.h      configuration struct
  rec-hud-overlay.*     overlay window (QPainter)
  rec-hud-settings.*    settings dialog
data/locale/            en-US.ini, it-IT.ini
scripts/                setup.ps1, build.ps1, install.ps1
```

## License

GPL-2.0-or-later, same as OBS Studio.
