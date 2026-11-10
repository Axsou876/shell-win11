<h1 align=center>caelestia-shell</h1>

<div align=center>

![GitHub last commit](https://img.shields.io/github/last-commit/caelestia-dots/shell?style=for-the-badge&labelColor=101418&color=9ccbfb)
![GitHub Repo stars](https://img.shields.io/github/stars/caelestia-dots/shell?style=for-the-badge&labelColor=101418&color=b9c8da)
![GitHub repo size](https://img.shields.io/github/repo-size/caelestia-dots/shell?style=for-the-badge&labelColor=101418&color=d3bfe6)
[![Ko-Fi donate](https://img.shields.io/badge/donate-kofi?style=for-the-badge&logo=ko-fi&logoColor=ffffff&label=ko-fi&labelColor=101418&color=f16061&link=https%3A%2F%2Fko-fi.com%2Fsoramane)](https://ko-fi.com/soramane)
[![Discord invite](https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fdiscordapp.com%2Fapi%2Finvites%2FBGDCFCmMBk%3Fwith_counts%3Dtrue&query=approximate_member_count&style=for-the-badge&logo=discord&logoColor=ffffff&label=discord&labelColor=101418&color=96f1f1&link=https%3A%2F%2Fdiscord.gg%2FBGDCFCmMBk)][discord]

</div>

https://github.com/user-attachments/assets/0840f496-575c-4ca6-83a8-87bb01a85c5f

## Caelestia Shell for Windows 11 — Phase 3 / Desktop Shell

This repository also contains a native Windows 11 port of the shell composition
(`windows/`). The Linux/Quickshell installation below is unchanged.

### Status

Phase 3 turns the Step 1e hover foundation into a functional desktop shell:

- **Launcher** (click the Caelestia logo, the search button, or `F1`): discovers
  Start Menu shortcuts and registered Windows/UWP applications through
  `shell:AppsFolder`, extracts real shell icons into a cache, supports
  case-insensitive fuzzy search, favourites, recent/frequency ordering, keyboard
  navigation and launches with `ShellExecuteExW`. Typed text is never executed
  as a command.
- **Left bar dock**: only applications explicitly marked as favourites are shown
  under the launcher/search button. The dock is hidden when there is no
  favourite, so no empty grid/icon remains. Clicking an icon launches it; the
  star in the launcher adds/removes it. Recent applications are not injected
  into the bar automatically.
- **Dashboard** (`F10`): Dashboard / Media / Performance / Workspaces tabs.
  Dashboard has a real weather card, Windows account profile picture/name,
  Windows version, hostname, uptime, calendar, media mini-player and live
  CPU/RAM/GPU/disk/network. Media has GSMTC artwork, title/artist/album,
  position/duration and controls. Performance uses circular live gauges.
  Workspaces lists the current desktop's windows and focuses them on click.
  No placeholder values are shown; unavailable counters display `N/D`.
- **Power menu**: a vertical floating menu on the right edge with the Windows
  account picture, sign-out, shutdown, settings and restart. Destructive actions
  require a second confirmation; Windows shutdown privilege is used.
- **Notifications** (bell button in the bar): a shell notification inbox with
  app/time/title/body, action buttons, close button, middle-click and horizontal
  swipe dismissal. A test notification is available from the empty state. This
  inbox is shell-owned; capturing Windows toasts from other applications is not
  available to an unpackaged Win32 build.
- **Wallpapers** (`>wal` in the launcher): scans real Pictures, Windows themes and
  Web wallpaper folders, shows thumbnails, filters as you type, marks the active
  wallpaper and applies the selection to the real Windows desktop through
  `SystemParametersInfoW(SPI_SETDESKWALLPAPER)`.
- **Control Centre** (`F9`): favourites, quick actions, real volume, network,
  Bluetooth and battery summaries, and session actions.
- **Bar popouts**: real Audio (`IAudioEndpointVolume`), Network (WLAN/IPHelper),
  Bluetooth (`Windows.Devices.Radios`) and Battery (`GetSystemPowerStatus`)
  panels. Power opens the right-edge session menu.
- **Motion**: SDF blob surface, spring deformation, panel slide/reveal,
  launcher result staggering, dashboard tab slide and icon hover/active states.
- The Windows AppBar, click-through overlays, focus rules, D3D11 path,
  multi-screen/DPI behaviour and `F8`/`F9`/`F10`/`Esc` from Step 1e are kept.

Still not implemented: capture of Windows toasts from other applications,
audio device selection, Bluetooth device list/pairing UI, power-profile
selection, HDR/high-contrast and touch/pen input.

### Build

Requirements: Windows 11, Qt 6.9.3 `msvc2022_64`, Visual Studio 2022 Build Tools
(MSVC C++20), CMake and Ninja.

```bat
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
cmake -S windows -B windows\build -G Ninja -DCMAKE_BUILD_TYPE=Debug ^
  -DCMAKE_PREFIX_PATH=C:\Users\<user>\Qt\6.9.3\msvc2022_64
cmake --build windows\build
```

Release:

```bat
cmake -S windows -B windows\build-release -G Ninja -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_PREFIX_PATH=C:\Users\<user>\Qt\6.9.3\msvc2022_64
cmake --build windows\build-release
```

### Diagnostics

The executable also exposes headless checks that use the same services as the
UI. Stop a running shell first if only one instance is desired.

```bat
caelestia-shell.exe --list-apps
caelestia-shell.exe --launch-app "Firefox"
caelestia-shell.exe --dump-system
caelestia-shell.exe --weather Paris
caelestia-shell.exe --dump-media
caelestia-shell.exe --media-toggle
```

`--launch-app` accepts only an application name discovered by the scanner; it
never executes free text.

Run `windows\build\bin\caelestia-shell.exe` with the Qt bin directory on `PATH`.
For a standalone folder, run `windeployqt` on that executable.

### Controls (Phase 3)

- Click the Caelestia logo in the bar, the search icon or press `F1`: open the
  application launcher. Type to filter; `↑`/`↓` navigate, `Enter` launches, `Esc`
  or a click outside closes. The star toggles a favourite.
- Type `>wal` in the launcher to switch to the wallpaper picker. `↑`/`↓`
  navigate, `Enter` applies the selected wallpaper to the desktop and closes the
  launcher, `Esc` closes it. Typing after `>wal` filters by name or folder.
- The left bar shows the favourite application dock and hides it when empty.
  The search icon opens the launcher, the bell opens notifications. There is no
  separate calendar button or empty application-grid icon in the bar.
- `F10` (shell focused) or hover the top-center strip: Dashboard. Its tabs are
  Dashboard, Media, Performance and Workspaces.
- `F9` (shell focused): Control Centre (favourites, quick actions, audio,
  network, Bluetooth, battery and power).
- Hover Audio, Network, Bluetooth or Battery in the bar: open the corresponding
  real panel after a 120 ms intent delay. Moving directly to another icon
  switches panels without an intermediate close.
- Move from the icon into the popout: it stays open. The 25 px halo covers the
  content, child buttons and the bar-to-panel gap. Leaving both the bar and the
  panel closes it after a 220 ms grace delay.
- Click a status icon: open the same panel explicitly and focus the overlay for
  keyboard input. Hover panels stay `NOACTIVATE`; the user's app keeps focus.
- The power button opens the right-edge session menu; shutdown/restart/sign-out
  require a second confirmation.
- `Esc`: closes the active surface through a global hotkey.
- `F8`: toggle the light/dark palette. `Ctrl+Q`: quit and release the AppBar.
  `F8`/`F9`/`F10` are Qt application shortcuts active while the shell overlay
  has focus, not global hotkeys, so other applications keep those keys.

### Architecture

- One always-visible overlay per monitor, full monitor width and Windows work
  area height, placed below the AppBar windows.
- `WS_EX_TRANSPARENT` + `WS_EX_NOACTIVATE` while closed: clicks pass through to
  other processes and the overlay never steals focus.
- Hover opens keep `WS_EX_TRANSPARENT` off but keep `WS_EX_NOACTIVATE`; the user's
  app keeps keyboard focus. Explicit clicks/F9/F10 clear both styles and activate
  the overlay for keyboard input. AppBars are re-raised above it.
- `PanelController.qml` owns the central hover state: 120 ms open delay, 220 ms
  close grace, per-icon target, cancellation and direct panel switching. A
  pending target blocks the close timer, so a switch never closes the old surface
  first. `openReason` and `currentScreenName` are updated before `currentPanel`,
  so `interactiveChanged` already knows a hover open and never clears
  `WS_EX_NOACTIVATE`.
- `BarStatusIcons.qml` maps the pointer to the real icon under the cursor and
  sends its name, screen and anchor. Leaving is handled centrally, not by the
  bar's `onExited` (which Qt synthesises when the panel is stacked above it).
- The 25 px hover halo lives in `OverlayWindow.checkHover`: every raw mouse move
  is compared with the active panel's expanded rectangle, the bar rail and the
  Dashboard trigger. Child buttons therefore stay inside the halo without
  stealing their clicks.
- The closed, click-through overlay is fed by Win32 raw mouse input
  (`RIDEV_INPUTSINK` on the AppBar) to detect the top-center Dashboard strip
  without polling or blocking clicks. This path responds to real/absolute mouse
  input; `SetCursorPos` alone does not generate raw input.
- `Esc` is registered as a window hotkey only while a panel is open, so a
  hover-opened panel can be dismissed without stealing focus.
- `ScreenSurface.qml` + `blob.vert/frag`: inverted 10 px frame, 25 px inner hole,
  circular smooth union, per-panel radii, 20 px smoothing, surface shadow.
- `SpringDeform.qml`: Caelestia BlobRect spring (stiffness 200, damping 16) with
  axis-aligned stretch/compress; the inverse matrix feeds the SDF and the forward
  scale transforms the panel content.
- Border sink: the inner wall recedes while a panel enters from a screen border.
- Sidebar 430 px (slide from right), Dashboard 854×560 (slide from top, animated
  tab indicator), popouts 320 px (fused into the bar with a 20% left extension).
- Popout widths use the original tokens: network/audio/Bluetooth 320 px, battery
  250 px.
- Strict monochrome palettes: pure black surfaces in dark mode, pure white in
  light mode, neutral greys for hierarchy and no coloured accent. The Caelestia
  mark keeps its two-tone rendering (primary + on-surface variant) so it does
  not collapse into a flat black or white silhouette. The theme follows the
  Windows app mode at startup; `F8` toggles it. Application icons, media artwork
  and the account picture keep their own colours.
- `AppLauncherService`: Start Menu `.lnk` files (`FOLDERID_Programs` and
  `FOLDERID_CommonPrograms`) plus registered applications (`FOLDERID_AppsFolder`,
  `PKEY_AppUserModel_ID`). `IShellLinkW` resolves target/arguments; shell icons
  are cached as PNG; search is local and case-insensitive/fuzzy; launching runs
  on a worker through `ShellExecuteExW`; favourites and usage counters live in
  `QSettings`.
- `SystemService`: `GetSystemTimes`, `GlobalMemoryStatusEx`, `GetIfTable2`,
  `GetDiskFreeSpaceExW`, DXGI video memory and PDH `GPU Engine` counters. GPU
  usage is marked unavailable when the PDH counter is absent.
- `MediaService`: Windows GSMTC via C++/WinRT, polled off the GUI thread;
  metadata, artwork cache and real play/pause/next/previous commands.
- `WeatherService`: Open-Meteo geocoding and forecast, no API key, city stored
  in `QSettings`; network errors and missing configuration are explicit.
- `AudioService`: Core Audio default render endpoint + `IAudioEndpointVolume`
  callbacks and setters. `NetworkService`: WLAN API/IPHelper with change
  notifications. `BluetoothService`: WinRT Radio state, events and set-state.
  `BatteryService`: `GetSystemPowerStatus` plus `WM_POWERBROADCAST`.
- `PowerService`: lock, sleep, sign-out, restart and shutdown with the Windows
  shutdown privilege. `PowerMenu.qml` is the right-edge vertical session menu;
  destructive actions require a second confirmation.
- `WallpaperService`: scans real wallpaper folders and applies the selection
  with `SystemParametersInfoW(SPI_SETDESKWALLPAPER)`; the selected path is kept
  in `QSettings`. `LauncherShell.qml` switches to its wallpaper list when the
  query starts with `>wal`.
- `NotificationService`: shell-owned notification inbox (title, app, body, time,
  actions); `NotificationsShell.qml` renders the cards, applies actions and
  dismisses with the close button, middle-click or a horizontal swipe.
- `UserService`: Windows account display name, avatar from AccountPictures (or
  the system generic picture), Windows version and hostname.
- `WindowService`: current-desktop top-level windows via `EnumWindows` and
  `IVirtualDesktopManager`; the Workspaces tab activates a window through
  `SetForegroundWindow`.
- `BarFavorites.qml`: vertical favourite dock in the left bar, real cached icons,
  launches through `Apps.launchById`. Only explicit favourites are shown; the
  dock is fully hidden when empty and recent applications are not added
  automatically. The bar also exposes the launcher search button. A slow timer
  re-raises the AppBar above maximized/borderless windows without changing its
  AppBar reservation.

### Known Windows differences

- The frame surrounds the Windows work area; the taskbar remains outside it.
- Fullscreen hiding, HDR/high contrast, touch/pen input and wallpaper-generated
  (Matugen) palettes are not implemented yet.
- The spring deformation is axis-aligned; detached/diagonal popouts would need
  the full symmetric 2x2 matrix and its content transform.
- Hover delays (120 ms open / 220 ms close) are a Windows adaptation; the Linux
  shell reacts immediately on pointer movement.
- Caelestia's Sidebar is a notification dock; the Windows port uses the same
  430 px surface as a Control Centre with real services and adds a separate
  Notifications panel. Windows toasts from other applications are not captured:
  an unpackaged Win32 build does not reliably get toast-listener access, so the
  inbox only shows notifications created by the shell (including its test
  notification).
- Bluetooth adapter state and on/off are real; pairing/device lists remain in
  Windows Settings. Audio supports the default output endpoint (volume/mute);
  selecting another output/input device remains in Windows Settings. Battery
  data is real; the Windows power overlay/profile is not switched by the shell.
- GSMTC only exposes players that publish a Windows media session. Apps without
  GSMTC support keep their own controls.
- CPU/GPU temperature is not exposed by a reliable Windows API here; the
  Performance gauges show real usage/memory rather than invented temperatures.
- The dashboard profile uses the Windows account picture when one exists;
  otherwise it falls back to the system generic account picture.
- Weather needs network access and a configured city; Open-Meteo is the only
  provider wired in this phase.
- Some registered UWP applications do not expose an icon through the shell
  image factory; the launcher falls back to a generic application icon.
- The original tray hover popout is not reproduced: the Windows port keeps the
  native Windows tray menu validated in earlier phases.
- One panel is active at a time. Hovering a bar icon while an explicit Sidebar or
  Dashboard is open switches panels; the original can keep some of them
  independent.
- Hover panels stay `NOACTIVATE`; their buttons work with the mouse, but a text
  field in a hover panel needs an explicit click/F9/F10 before it gets keyboard
  focus.
- Raw-input Dashboard hover responds to real or absolute injected mouse input;
  `SetCursorPos` alone does not generate Win32 raw input, and a relative
  `SendInput` immediately after a cursor warp may be coalesced. Automated tests
  should use absolute `SendInput`.
- Performance measurements are indicative (QML frame meter + GPU Engine
  counters), not a dedicated profiler. The global MultiEffect shadow remains the
  main per-frame cost.

## Components

- Widgets: [`Quickshell`](https://quickshell.outfoxxed.me)
- Window manager: [`Hyprland`](https://hypr.land)
- Dots: [`caelestia`][dots-repo]

## Installation

> [!NOTE]
> This repo is for Caelestia's desktop shell only. If you want installation instructions
> for the entire dotfiles (which include this shell), head to [the main repo][dots-repo] instead.

### Arch Linux

> [!WARNING]
> If you want to make your own changes/tweaks to the shell, do NOT edit the files installed by the AUR
> package. Instead, follow the instructions in the [manual installation section](#manual-installation).

The shell is available from the AUR as `caelestia-shell`. You can install it with an AUR helper (recommended),
like [`paru`](https://github.com/morganamilo/paru), or by manually downloading the PKGBUILD and running `makepkg -si`.

A package following the latest commit also exists as `caelestia-shell-git`. This is bleeding-edge
and likely to be unstable/have bugs. Regular users are recommended to use the stable package (`caelestia-shell`).

### Nix

You can run the shell directly via `nix run`:

```sh
nix run github:caelestia-dots/shell#with-cli
```

Or add it to your system configuration:

```nix
{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";

    caelestia-shell = {
      url = "github:caelestia-dots/shell";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };
}
```

For full functionality, use `caelestia-shell.packages.<system>.with-cli`, which can be added to your
`environment.systemPackages`, `users.users.<username>.packages`, `home.packages` if using home-manager,
or a devshell. The `default` package does not include the CLI.
You can then run the shell with `caelestia-shell`.

For home-manager, you can also use Caelestia's Home Manager module (explained in [the configuration section](#home-manager-module)), which installs and configures the shell and CLI.

### Manual installation

Dependencies:

- [`caelestia-cli`](https://github.com/caelestia-dots/cli)
- [`quickshell-git`](https://git.outfoxxed.me/quickshell/quickshell) - this has to be the git version, not the latest tagged version
- `glibc`
- `gcc-libs`
- [`ddcutil`](https://github.com/rockowitz/ddcutil)
- [`brightnessctl`](https://github.com/Hummer12007/brightnessctl)
- [`libcava`](https://github.com/LukashonakV/cava)
- [`networkmanager`](https://gitlab.freedesktop.org/NetworkManager/NetworkManager)
- [`lm_sensors`](https://github.com/lm-sensors/lm-sensors)
- [`aubio`](https://github.com/aubio/aubio)
- [`libpipewire`](https://github.com/PipeWire/pipewire)
- [`libqalculate`](https://github.com/Qalculate/libqalculate)
- [`power-profiles-daemon`](https://gitlab.freedesktop.org/upower/power-profiles-daemon)
- [`ttf-material-symbols-variable`](https://github.com/google/material-design-icons)
- [`ttf-rubik-vf`](https://github.com/googlefonts/rubik)
- [`ttf-cascadia-code-nerd`](https://github.com/ryanoasis/nerd-fonts)
- `qt6-base`
- `qt6-declarative`
- `qt6-imageformats`
- `qt6-multimedia`
- [`qt6-m3shapes-git`](https://github.com/soramanew/m3shapes)
- [`swappy`](https://github.com/jtheoof/swappy)
- [`fish`](https://github.com/fish-shell/fish-shell)
- [`bash`](https://www.gnu.org/software/bash)

Build dependencies:

- [`cmake`](https://gitlab.kitware.com/cmake/cmake)
- [`ninja`](https://github.com/ninja-build/ninja)
- `qt6-shadertools`

> [!IMPORTANT]
> The commands below (and in the "Updating" section) assume `$XDG_CONFIG_HOME` is set.
> If it is unset, substitute it with the path to your config folder (typically `~/.config`).

To install the shell manually, install all dependencies and clone this repo to `$XDG_CONFIG_HOME/quickshell/caelestia`.
Then build and install using CMake.

```sh
cd $XDG_CONFIG_HOME/quickshell
git clone https://github.com/caelestia-dots/shell.git caelestia

cd caelestia
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/
cmake --build build
sudo cmake --install build
```

> [!TIP]
> You can customise the installation location via the CMake flags `INSTALL_LIBDIR`, `INSTALL_QMLDIR`, and
> `INSTALL_QSCONFDIR` for the libraries (e.g. the version helper), QML plugin, and Quickshell config directories
> respectively. If you set the `INSTALL_LIBDIR` flag, the `CAELESTIA_LIB_DIR` variable must also be set to
> the same directory in your system's environment.
>
> For example, installing to `~/.config/quickshell/caelestia` for easy local changes:
>
> ```sh
> mkdir -p ~/.config/quickshell/caelestia
> cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/ -DINSTALL_QSCONFDIR="$HOME/.config/quickshell/caelestia"
> cmake --build build
> sudo cmake --install build
> sudo chown -R $USER ~/.config/quickshell/caelestia
> ```

## Usage

You can start the shell by running `caelestia shell -d` (preferred) or `qs -c caelestia -n -d`.
You may omit `-d` from the command to keep the shell attached to the current terminal if necessary,
though you likely want it to be detached (so it doesn't close when the terminal is closed).

If using the [Caelestia dotfiles][dots-repo], the shell will be autostarted on login
via a `hl.on("hyprland.start", ...)` function in the Hyprland config.

### Shortcuts/IPC

All keybinds are accessible via Hyprland [global shortcuts](https://wiki.hypr.land/Configuring/Basics/Binds/#dbus-global-shortcuts).
If using the [Caelestia dotfiles][dots-repo], the keybinds are already configured for you.
Otherwise, the [`keybinds.lua`](https://github.com/caelestia-dots/caelestia/blob/main/hypr/hyprland/keybinds.lua#L63-L67) file
contains an example of how to use global shortcuts.

All IPC commands can be accessed via `caelestia shell ...`, for example:

```sh
caelestia shell mpris getActive trackTitle
```

You can view the list of available IPC commands by running `caelestia shell -s`.

### PFP/Wallpapers

The profile picture for the dashboard is read from the file `~/.face`. You can set it by clicking it in the dashboard,
or by manually copying or symlinking your image to the path.

The wallpapers for the wallpaper switcher are read from `~/Pictures/Wallpapers`
by default. To change it, modify `paths.wallpaperDir` in `~/.config/caelestia/shell.json`.

To set the wallpaper, you can type `>wallpaper` in the launcher to open the wallpaper switcher.
Alternatively, you can also use `caelestia wallpaper -f <path_to_wallpaper>` to set the wallpaper directly.
Use `caelestia wallpaper -h` for more info about this command.

## Updating

### Packaged install (AUR)

If using the full dotfiles or the CLI, run `caelestia update` to perform a full system update and
update the dots.
Otherwise, if you installed the shell on its own, update your system using your AUR helper (e.g., `paru`).

### Manual install

If you installed the shell manually by cloning the repo, you can update by pulling the changes from git
in the local checkout.

For example, if you installed to `$XDG_CONFIG_HOME/quickshell/caelestia`:

```sh
cd $XDG_CONFIG_HOME/quickshell/caelestia
git pull
```

## Configuring

All configuration options belong in `~/.config/caelestia/shell.json`. This file is _not_ created by
default; you must create it manually. Options that you omit from the config file will use their default
values.

### Per-monitor configuration

You can configure per-monitor options in `~/.config/caelestia/monitors/<monitor_name>/shell.json`.
List the names of your available monitors by running:

```sh
hyprctl monitors -j | jq -r '.[].name'
```

Options set in these files will **override** the respective options in the global config. Any options not present in
per-monitor configs will inherit their values from the global config.

For example, to automatically hide the bar on the monitor named `DP-1`:

**`~/.config/caelestia/monitors/DP-1/shell.json`**

```json
{
    "bar": {
        "persistent": false
    }
}
```

> [!NOTE]
> Not all options respect per-monitor overrides. Most notably, the following options will only read
> from the global config, and ignore the respective option in per-monitor config files.
>
> <details><summary>Ignored options</summary>
>
> - `appearance`: `anim.*`, `transparency.*`
> - `bar.tray`: `hiddenIcons`, `iconSubs`
> - `bar.workspaces`: `ignoredTags`, `specialWorkspaceIcons`, `windowIcons`, `workspaceIcons`
> - `dashboard`: `mediaUpdateInterval`, `resourceUpdateInterval`
> - `general`: `apps.*`, `battery.*`, `idle.*`, `logo`
> - `launcher`: `actionPrefix`, `actions`, `enableDangerousActions`, `favouriteApps`, `hiddenApps`, `specialPrefix`, `useFuzzy.*`, `vimKeybinds`
> - `lock`: `enableFprint`, `enableHowdy`, `maxFprintTries`, `maxHowdyTries`, `triggerHowdyOnWake`
> - `nexus`: `networkRescanInterval`
> - `notifs`: `actionOnClick`, `defaultExpireTimeout`, `expire`, `fullscreen`, `fullscreenExpireTimeout`
> - `paths`: `lyricsDir`, `wallpaperDir`
> - `services`: `audioIncrement`, `brightnessIncrement`, `clockFormat`, `dataUnits`, `defaultPlayer`, `gpuType`, `lyricsBackend`, `maxVolume`, `playerAliases`, `sensorUnits`, `smartScheme`, `visualiserBars`, `weatherLocation`, `weatherUnits`
> - `utilities`: `gameMode.*`, `toasts.*`, `vpn.*`
>
> </details>

### Example configuration

> [!WARNING]
> The example configuration includes **ALL** configuration options in `shell.json`. It is
> **not** recommended to copy and paste this entire configuration into `shell.json`,
> as options or their default values may change across updates, resulting in a stale config.
>
> This is meant to serve as a reference of all the available options, and you should
> <ins>only add the ones you want to change</ins> to `shell.json`.

<details><summary>Example config</summary>

```json
{
    "enabled": true,
    "appearance": {
        "deformScale": 1,
        "rounding": {
            "scale": 1
        },
        "spacing": {
            "scale": 1
        },
        "padding": {
            "scale": 1
        },
        "font": {
            "scale": 1,
            "clock": "Rubik",
            "workspaces": "Rubik",
            "headline": {
                "family": "GoogleSansFlex",
                "large": {
                    "size": 32,
                    "weight": 500,
                    "italic": false,
                    "vaxes": { "ROND": 25 }
                },
                "medium": {
                    "size": 28,
                    "weight": 500,
                    "italic": false,
                    "vaxes": { "ROND": 25 }
                },
                "small": {
                    "size": 24,
                    "weight": 500,
                    "italic": false,
                    "vaxes": { "ROND": 25 }
                }
            },
            "title": {
                "family": "GoogleSansFlex",
                "large": {
                    "size": 22,
                    "weight": 500,
                    "italic": false,
                    "vaxes": { "ROND": 25 }
                },
                "medium": {
                    "size": 16,
                    "weight": 500,
                    "italic": false,
                    "vaxes": { "ROND": 25 }
                },
                "small": {
                    "size": 14,
                    "weight": 500,
                    "italic": false,
                    "vaxes": { "ROND": 25 }
                }
            },
            "body": {
                "family": "GoogleSansFlex",
                "large": {
                    "size": 16,
                    "weight": 400,
                    "italic": false,
                    "vaxes": { "ROND": 25 }
                },
                "medium": {
                    "size": 14,
                    "weight": 400,
                    "italic": false,
                    "vaxes": { "ROND": 25 }
                },
                "small": {
                    "size": 12,
                    "weight": 400,
                    "italic": false,
                    "vaxes": { "ROND": 25 }
                }
            },
            "label": {
                "family": "GoogleSansFlex",
                "large": {
                    "size": 14,
                    "weight": 500,
                    "italic": false,
                    "vaxes": { "ROND": 25 }
                },
                "medium": {
                    "size": 12,
                    "weight": 500,
                    "italic": false,
                    "vaxes": { "ROND": 25 }
                },
                "small": {
                    "size": 11,
                    "weight": 400,
                    "italic": false,
                    "vaxes": { "ROND": 25 }
                }
            },
            "mono": {
                "family": "CaskaydiaCove NF",
                "large": {
                    "size": 16,
                    "weight": 400,
                    "italic": false,
                    "vaxes": {}
                },
                "medium": {
                    "size": 14,
                    "weight": 400,
                    "italic": false,
                    "vaxes": {}
                },
                "small": {
                    "size": 12,
                    "weight": 400,
                    "italic": false,
                    "vaxes": {}
                }
            },
            "icon": {
                "family": "Material Symbols Rounded",
                "extraLarge": {
                    "size": 36,
                    "weight": 400,
                    "italic": false,
                    "vaxes": {}
                },
                "large": {
                    "size": 24,
                    "weight": 400,
                    "italic": false,
                    "vaxes": {}
                },
                "medium": {
                    "size": 18,
                    "weight": 400,
                    "italic": false,
                    "vaxes": {}
                },
                "small": {
                    "size": 15,
                    "weight": 400,
                    "italic": false,
                    "vaxes": {}
                }
            }
        },
        "anim": {
            "durations": {
                "scale": 1
            }
        },
        "transparency": {
            "enabled": false,
            "base": 0.85,
            "layers": 0.4
        }
    },
    "general": {
        "logo": "",
        "showOverFullscreen": false,
        "mediaGifSpeedAdjustment": 300,
        "sessionGifSpeed": 0.7,
        "apps": {
            "terminal": ["foot"],
            "audio": ["pwvucontrol"],
            "playback": ["mpv"],
            "explorer": ["thunar"]
        },
        "idle": {
            "lockBeforeSleep": true,
            "inhibitWhenAudio": true,
            "inhibitWhenCharging": false,
            "timeouts": [
                {
                    "timeout": 180,
                    "idleAction": "lock",
                    "inhibitWhenAudio": false,
                    "inhibitWhenCharging": false,
                    "respectInhibitors": true
                },
                {
                    "timeout": 300,
                    "idleAction": "dpms off",
                    "returnAction": "dpms on"
                },
                {
                    "timeout": 600,
                    "idleAction": ["suspendThenHibernate"]
                }
            ]
        },
        "battery": {
            "warnLevels": [
                {
                    "level": 20,
                    "title": "Low battery",
                    "message": "You might want to plug in a charger",
                    "icon": "battery_android_frame_2"
                },
                {
                    "level": 10,
                    "title": "Did you see the previous message?",
                    "message": "You should probably plug in a charger <b>now</b>",
                    "icon": "battery_android_frame_1"
                },
                {
                    "level": 5,
                    "title": "Critical battery level",
                    "message": "PLUG THE CHARGER RIGHT NOW!!",
                    "icon": "battery_android_alert",
                    "critical": true
                }
            ],
            "criticalLevel": 3
        }
    },
    "background": {
        "enabled": true,
        "wallpaperEnabled": true,
        "desktopClock": {
            "enabled": false,
            "scale": 1.0,
            "position": "bottom-right",
            "invertColors": false,
            "background": {
                "enabled": false,
                "opacity": 0.7,
                "blur": true
            },
            "shadow": {
                "enabled": true,
                "opacity": 0.7,
                "blur": 0.4
            }
        },
        "visualiser": {
            "enabled": false,
            "autoHide": true,
            "blur": false,
            "rounding": 1,
            "spacing": 1
        }
    },
    "bar": {
        "persistent": true,
        "showOnHover": true,
        "dragThreshold": 20,
        "scrollActions": {
            "workspaces": true,
            "volume": true,
            "brightness": true
        },
        "popouts": {
            "activeWindow": true,
            "tray": true,
            "statusIcons": true
        },
        "workspaces": {
            "shown": 5,
            "activeIndicator": true,
            "occupiedBg": false,
            "showUnoccupied": true,
            "perMonitor": true,
            "showWindows": true,
            "showWindowsOnSpecialWorkspaces": true,
            "maxWindowIcons": 5,
            "activeTrail": false,
            "displayType": "shapes",
            "specialDisplayType": "icons",
            "label": "  ",
            "occupiedLabel": "󰮯",
            "activeLabel": "󰮯",
            "capitalisation": "preserve",
            "workspaceIcons": [],
            "specialWorkspaceIcons": [
                {
                    "name": "special",
                    "icon": "star"
                },
                {
                    "name": "communication",
                    "icon": "forum"
                },
                {
                    "name": "music",
                    "icon": "music_cast"
                },
                {
                    "name": "todo",
                    "icon": "checklist"
                },
                {
                    "name": "sysmon",
                    "icon": "monitor_heart"
                }
            ],
            "ignoredTags": ["hide_in_bar", "xwl_popup"],
            "windowIcons": [
                {
                    "regex": "steam(_app_(default|[0-9]+))?",
                    "icon": "sports_esports"
                }
            ]
        },
        "activeWindow": {
            "compact": false,
            "inverted": false,
            "showOnHover": true
        },
        "tray": {
            "background": false,
            "recolour": false,
            "compact": false,
            "iconSubs": [],
            "hiddenIcons": []
        },
        "clock": {
            "background": false,
            "showDate": false,
            "showIcon": true
        },
        "statusIcons": [
            {
                "id": "lockStatus",
                "enabled": true
            },
            {
                "id": "audio",
                "enabled": false
            },
            {
                "id": "microphone",
                "enabled": false
            },
            {
                "id": "kbLayout",
                "enabled": false
            },
            {
                "id": "network",
                "enabled": true
            },
            {
                "id": "bluetooth",
                "enabled": true
            },
            {
                "id": "battery",
                "enabled": true
            }
        ],
        "entries": [
            {
                "id": "logo",
                "enabled": true
            },
            {
                "id": "workspaces",
                "enabled": true
            },
            {
                "id": "spacer",
                "enabled": true
            },
            {
                "id": "activeWindow",
                "enabled": true
            },
            {
                "id": "spacer",
                "enabled": true
            },
            {
                "id": "tray",
                "enabled": true
            },
            {
                "id": "clock",
                "enabled": true
            },
            {
                "id": "statusIcons",
                "enabled": true
            },
            {
                "id": "power",
                "enabled": true
            }
        ],
        "excludedScreens": []
    },
    "border": {
        "thickness": 10,
        "rounding": 25,
        "smoothing": 20
    },
    "dashboard": {
        "enabled": true,
        "showOnHover": true,
        "showDashboard": true,
        "showMedia": true,
        "showPerformance": true,
        "showWeather": true,
        "mediaUpdateInterval": 500,
        "resourceUpdateInterval": 1000,
        "dragThreshold": 50,
        "performance": {
            "showBattery": true,
            "showGpu": true,
            "showCpu": true,
            "showMemory": true,
            "showStorage": true,
            "showNetwork": true
        }
    },
    "launcher": {
        "enabled": true,
        "showOnHover": false,
        "maxShown": 7,
        "maxWallpapers": 9,
        "specialPrefix": "@",
        "actionPrefix": ">",
        "enableDangerousActions": false,
        "dragThreshold": 50,
        "vimKeybinds": false,
        "favouriteApps": [],
        "hiddenApps": [],
        "useFuzzy": {
            "apps": false,
            "actions": false,
            "schemes": false,
            "variants": false,
            "wallpapers": false
        },
        "actions": [
            {
                "name": "Calculator",
                "icon": "calculate",
                "description": "Do simple math equations (powered by Qalc)",
                "command": ["autocomplete", "calc"],
                "enabled": true,
                "dangerous": false
            },
            {
                "name": "Scheme",
                "icon": "palette",
                "description": "Change the current colour scheme",
                "command": ["autocomplete", "scheme"],
                "enabled": true,
                "dangerous": false
            },
            {
                "name": "Wallpaper",
                "icon": "image",
                "description": "Change the current wallpaper",
                "command": ["autocomplete", "wallpaper"],
                "enabled": true,
                "dangerous": false
            },
            {
                "name": "Variant",
                "icon": "colors",
                "description": "Change the current scheme variant",
                "command": ["autocomplete", "variant"],
                "enabled": true,
                "dangerous": false
            },
            {
                "name": "Random",
                "icon": "casino",
                "description": "Switch to a random wallpaper",
                "command": ["caelestia", "wallpaper", "-r"],
                "enabled": true,
                "dangerous": false
            },
            {
                "name": "Light",
                "icon": "light_mode",
                "description": "Change the scheme to light mode",
                "command": ["setMode", "light"],
                "enabled": true,
                "dangerous": false
            },
            {
                "name": "Dark",
                "icon": "dark_mode",
                "description": "Change the scheme to dark mode",
                "command": ["setMode", "dark"],
                "enabled": true,
                "dangerous": false
            },
            {
                "name": "Shutdown",
                "icon": "power_settings_new",
                "description": "Shutdown the system",
                "command": ["poweroff"],
                "enabled": true,
                "dangerous": true
            },
            {
                "name": "Reboot",
                "icon": "cached",
                "description": "Reboot the system",
                "command": ["reboot"],
                "enabled": true,
                "dangerous": true
            },
            {
                "name": "Logout",
                "icon": "exit_to_app",
                "description": "Log out of the current session",
                "command": ["logout"],
                "enabled": true,
                "dangerous": true
            },
            {
                "name": "Lock",
                "icon": "lock",
                "description": "Lock the current session",
                "command": ["loginctl", "lock-session"],
                "enabled": true,
                "dangerous": false
            },
            {
                "name": "Sleep",
                "icon": "bedtime",
                "description": "Suspend then hibernate",
                "command": ["suspendThenHibernate"],
                "enabled": true,
                "dangerous": false
            },
            {
                "name": "Settings",
                "icon": "settings",
                "description": "Configure the shell",
                "command": ["caelestia", "shell", "nexus", "open"],
                "enabled": true,
                "dangerous": false
            }
        ]
    },
    "lock": {
        "enabled": true,
        "useWallpaper": false,
        "recolourLogo": true,
        "enableFprint": true,
        "maxFprintTries": 3,
        "enableHowdy": true,
        "maxHowdyTries": 3,
        "triggerHowdyOnWake": true,
        "hideNotifs": false,
        "enableSessionControls": true
    },
    "nexus": {
        "wallpapersPerRow": 4,
        "networkRescanInterval": 15000
    },
    "notifs": {
        "expire": true,
        "fullscreen": "On",
        "defaultExpireTimeout": 5000,
        "fullscreenExpireTimeout": 2000,
        "clearThreshold": 0.3,
        "expandThreshold": 20,
        "actionOnClick": false,
        "groupPreviewNum": 3,
        "openExpanded": false
    },
    "osd": {
        "enabled": true,
        "hideDelay": 2000,
        "enableBrightness": true,
        "enableMicrophone": false
    },
    "services": {
        "weatherLocation": "",
        "weatherUnits": "Auto",
        "sensorUnits": "Celsius",
        "dataUnits": "Binary",
        "clockFormat": "Auto",
        "gpuType": "Auto",
        "visualiserBars": 60,
        "audioIncrement": 0.1,
        "brightnessIncrement": 0.1,
        "maxVolume": 1.0,
        "smartScheme": true,
        "defaultPlayer": "Spotify",
        "playerAliases": [
            { "from": "com.github.th_ch.youtube_music", "to": "YT Music" }
        ],
        "lyricsBackend": "Auto"
    },
    "session": {
        "enabled": true,
        "dragThreshold": 30,
        "vimKeybinds": false,
        "icons": {
            "logout": "logout",
            "shutdown": "power_settings_new",
            "hibernate": "downloading",
            "reboot": "cached"
        },
        "commands": {
            "logout": ["logout"],
            "shutdown": ["poweroff"],
            "hibernate": ["hibernate"],
            "reboot": ["reboot"]
        }
    },
    "sidebar": {
        "enabled": true,
        "showOnHover": false,
        "minHoverThreshold": 200,
        "dragThreshold": 80
    },
    "utilities": {
        "enabled": true,
        "maxToasts": 4,
        "toasts": {
            "fullscreen": "off",
            "configLoaded": true,
            "chargingChanged": true,
            "gameModeChanged": true,
            "dndChanged": true,
            "audioOutputChanged": true,
            "audioInputChanged": true,
            "capsLockChanged": true,
            "numLockChanged": true,
            "kbLayoutChanged": true,
            "kbLimit": true,
            "vpnChanged": true,
            "nowPlaying": false
        },
        "vpn": {
            "enabled": false,
            "provider": [
                {
                    "name": "wireguard",
                    "interface": "your-connection-name",
                    "displayName": "Wireguard (Your VPN)",
                    "enabled": false
                }
            ]
        },
        "quickToggles": [
            {
                "id": "wifi",
                "enabled": true
            },
            {
                "id": "bluetooth",
                "enabled": true
            },
            {
                "id": "mic",
                "enabled": true
            },
            {
                "id": "settings",
                "enabled": true
            },
            {
                "id": "gameMode",
                "enabled": true
            },
            {
                "id": "dnd",
                "enabled": true
            },
            {
                "id": "vpn",
                "enabled": false
            }
        ],
        "gameMode": {
            "options": {
                "animations:enabled": 0,
                "decoration:blur:enabled": 0,
                "decoration:shadow:enabled": 0,
                "decoration:rounding": 0,
                "general:gaps_in": 0,
                "general:gaps_out": 0,
                "general:border_size": 1,
                "general:allow_tearing": 1
            }
        }
    },
    "paths": {
        "wallpaperDir": "~/Pictures/Wallpapers",
        "lyricsDir": "~/Music/lyrics/",
        "sessionGif": "root:/assets/kurukuru.gif",
        "mediaGif": "root:/assets/bongocat.gif",
        "noNotifsPic": "root:/assets/dino.png",
        "lockNoNotifsPic": "root:/assets/dino.png"
    }
}
```

</details>

### Advanced configuration

> [!CAUTION]
> Do NOT change any of these options unless you know what you are doing. These options control the
> tokens used internally within the shell, and can cause visual issues if modified incorrectly.
> The available options may change or be removed without notice across versions.

A separate `~/.config/caelestia/shell-tokens.json` file allows editing the internal tokens without
touching the source code of the shell. These tokens affect the dimensions and appearance of visual elements,
including individual rounding, spacing, padding, font size, animation durations and curves, and the sizes of
certain components. The appearance scale values in `shell.json` are multiplied against these base
token values to produce the final computed values.

Per-monitor token overrides are also available at
`~/.config/caelestia/monitors/<monitor_name>/shell-tokens.json`.

### Home Manager Module

For NixOS users, a Home Manager module is also available.

<details><summary><code>home.nix</code></summary>

```nix
programs.caelestia = {
  enable = true;
  systemd = {
    enable = false; # if you prefer starting from your compositor
    target = "graphical-session.target";
    environment = [];
  };
  settings = {
    bar.statusIcons = [
      { id = "lockStatus"; enabled = true; }
      { id = "network"; enabled = true; }
      { id = "bluetooth"; enabled = true; }
      { id = "battery"; enabled = false; }
    ];
    paths.wallpaperDir = "~/Images";
  };
  cli = {
    enable = true; # Also add caelestia-cli to path
    settings = {
      theme.enableGtk = false;
    };
  };
};
```

The module automatically adds the shell to the path with **full functionality**. The CLI is not required; however, you can enable and configure it.

</details>

## FAQ

### Need help or support?

You can join the Caelestia Discord server for assistance and discussion [here][discord].

### I want to make my own changes to the Hyprland config!

Check out the configuring section on the [dots repo](https://github.com/caelestia-dots/caelestia#configuring).

### I want to make my own changes to other stuff!

See the [manual installation](#manual-installation) section for the corresponding repo.

### I want to disable \_\_\_ feature!

Please read the [configuring](#configuring) section.
If there is no corresponding option, make a [feature request](https://github.com/caelestia-dots/shell/issues/new?template=feature.yml).

### How do I make my colour scheme change to match my wallpaper?

Set a wallpaper via `>wallpaper` in the launcher or `caelestia wallpaper`, and set the scheme to the dynamic scheme via
`>scheme` in the launcher or `caelestia scheme set`, e.g.:

```sh
caelestia wallpaper -f <path_to_wallpaper>
caelestia scheme set -n dynamic
```

### My wallpapers aren't showing up in the launcher!

The launcher pulls wallpapers from `~/Pictures/Wallpapers` by default. You can change this in the config. Additionally,
the launcher only shows an odd number of wallpapers at one time. If you only have 2 wallpapers, consider getting more
(or just putting one).

## Credits

Thanks to the Hyprland Discord community (especially the homies in #rice-discussion) for all the help and suggestions
for improving these dots!

A special thanks to [@outfoxxed](https://github.com/outfoxxed) for making Quickshell and the effort put into fixing issues
and implementing various feature requests.

Another special thanks to [@end_4](https://github.com/end-4) for his [config](https://github.com/end-4/dots-hyprland)
which helped me a lot with learning how to use Quickshell.

Finally, another thank you to all the configs I took inspiration from (only one for now):

- [Axenide/Ax-Shell](https://github.com/Axenide/Ax-Shell)

## Stonks 📈

<a href="https://www.star-history.com/#caelestia-dots/shell&Date">
    <picture>
        <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/svg?repos=caelestia-dots/shell&type=Date&theme=dark" />
        <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/svg?repos=caelestia-dots/shell&type=Date" />
        <img alt="Star History Chart" src="https://api.star-history.com/svg?repos=caelestia-dots/shell&type=Date" />
    </picture>
</a>

[dots-repo]: https://github.com/caelestia-dots/caelestia
[discord]: https://caelestiashell.com/discord
