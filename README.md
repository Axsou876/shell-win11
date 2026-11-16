# Caelestia Shell for Windows 11

A native Windows 11 port of [Caelestia](https://github.com/caelestia-dots/shell),
the Qt/Quickshell desktop shell built for Hyprland. Written in C++20 and Qt 6
Quick — no Electron, no scripting runtime, one process.

![Caelestia Shell on Windows](windows/caelestia-phase15.png)

## What it does

- **Left bar** — launcher, virtual desktops, active window, tray, clock, audio,
  Bluetooth, network, battery, power.
- **Application dock** at the bottom, hidden until the pointer reaches it: one
  icon per open window, click to focus, click again to minimize.
- **Dashboard** — overview, media (cover visualiser and synced lyrics),
  performance, weather, workspaces.
- **Launcher** — start menu and UWP apps, fuzzy search, favourites, real icons.
- **Notifications / control centre** — opens when the pointer touches the right
  edge of the screen.
- **Lock screen**, power menu, volume and brightness OSD.
- **Settings panel** — wallpaper, 12 editor/accent palettes plus a free colour
  picker, and a shell opacity slider. A colour change repaints the whole shell
  instantly.
- **Window management** — the shell keeps ownership of the Windows key, adds
  Hyprland-style `Win+…` shortcuts, and steps aside for fullscreen apps.

## Shortcuts

| Keys | Action |
|---|---|
| `Win` (tap) | launcher |
| `Win+Q` / `Win+Enter` | terminal |
| `Win+C` | close the active window |
| `Win+E` | Explorer |
| `Win+F` | fullscreen the active window |
| `Win+↑` / `Win+↓` | maximize / restore or minimize |
| `Win+1…0`, `Win+Shift+1…0` | switch desktop / move the window to it |
| `Win+Alt+L` | lock the session |
| `Win+←` / `Win+→` | Windows' own snap, left unbound on purpose |

## Requirements

- Windows 11. Virtual desktops need **24H2 or newer** (the private COM interface
  changed); on other builds the workspace column simply stays hidden.
- Visual Studio 2022 Build Tools (MSVC), Qt 6.9 for MSVC 2022 x64 (with Shader
  Tools and SVG), CMake 3.21+, Ninja.

## Build

```powershell
powershell -ExecutionPolicy Bypass -File windows\scripts\build.ps1 -Clean -Deploy
```

Or by hand:

```powershell
cmake -S windows -B windows\build-release -G Ninja `
  -DCMAKE_PREFIX_PATH=C:/Qt/6.9.3/msvc2022_64 -DCMAKE_BUILD_TYPE=Release
cmake --build windows\build-release
```

## Run

```
windows\build-release\bin\caelestia-shell.exe
```

The executable is a GUI binary: launching it opens **no console window**. Only
one instance runs at a time. Right-click the tray icon for **Start with Windows**
(writes the usual per-user `Run` entry) and **Quit**.

## Good to know

- The shell hides the Windows taskbar while it runs. Closing the shell — or
  starting it again after a crash — restores the previous state.
- Fullscreen applications are left alone: the shell steps aside instead of
  drawing over the video.
- Settings are stored per user under `HKCU\Software\Caelestia`.
- The port targets Windows 11 and is developed on 24H2/26H2.

## Documentation

- `windows/README.md` — detailed technical notes, in French: architecture,
  services, measurements, known limits.
- `README-upstream.md` — the original Caelestia documentation (Linux/Quickshell),
  kept unchanged.

## Credits

Caelestia is by [soramane](https://github.com/caelestia-dots) and the Caelestia
contributors. This port reuses its layout, tokens and assets and is released
under the same licence: **GPL-3.0** (see `LICENSE`).
