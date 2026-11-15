pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Window
import Caelestia.Platform 1.0
import "overlay"
import "lock"

Item {
    id: app

    visible: false
    width: 1
    height: 1

    // The palette is one shared object and every window binds to it, so a
    // scheme change repaints the bar, the panels, the popouts and the lock
    // screen at once instead of only the surface that picked it.
    property string schemeKey: Platform.shellSetting("appearance/scheme", "monochrome")
    // Arbitrary colour behind the "Perso" scheme, written by the colour
    // dialog in Nexus > Apparence.
    property string accentKey: Platform.shellSetting("appearance/accent", "#5b9dff")
    // Nexus > Système: keep every shell window rendering at the display
    // refresh instead of on demand. Off by default: an idle shell has nothing
    // to redraw, and the extra frames cost battery.
    property bool smoothRefresh: Platform.shellSetting("refresh/smooth", false)
    // See-through amount of the shell surface, written by the opacity slider.
    property real opacityKey: {
        const value = Number(Platform.shellSetting("appearance/opacity", 0.85));
        return isNaN(value) ? 0.85 : Math.max(0.3, Math.min(1, value));
    }

    Theme {
        id: appTheme

        dark: Platform.darkMode
        // Persisted in Nexus > Apparence. Reading it here (instead of inside
        // Theme) keeps Theme free of the platform singleton, so it stays a
        // plain palette object.
        scheme: app.schemeKey
        customAccent: app.accentKey
        surfaceOpacity: app.opacityKey
        systemAccent: Platform.accentColor
    }

    // shellSetting() is an invokable, so a binding on it has nothing to
    // invalidate. The store signals the write instead and the key is re-read
    // here; that also keeps Nexus from assigning theme.scheme directly, which
    // used to break the binding and lose the value.
    Connections {
        target: Platform

        function onShellSettingChanged(key) {
            if (key === "appearance/scheme")
                app.schemeKey = Platform.shellSetting(key, "monochrome");
            else if (key === "appearance/accent")
                app.accentKey = Platform.shellSetting(key, "#5b9dff");
            else if (key === "appearance/opacity")
                app.opacityKey = Number(Platform.shellSetting(key, 0.85));
            else if (key === "refresh/smooth")
                app.smoothRefresh = Platform.shellSetting(key, false);
        }
    }

    PanelController {
        id: panelController
    }

    // Session lock screen (Win+Alt+L, power menu, launcher). Declared before
    // the Instantiators so their delegates can bind to its state from the
    // first frame.
    LockOverlay {
        id: lockOverlay

        theme: appTheme
    }

    Connections {
        target: WindowManager

        function onLauncherRequested() {
            panelController.togglePanel("launcher", WindowManager.screenName(), 0);
        }

        function onSidebarRequested() {
            panelController.togglePanel("sidebar", WindowManager.screenName(), 0);
        }

        function onSessionRequested() {
            panelController.togglePanel("power", WindowManager.screenName(), 0);
        }

        // The lock screen captures the desktop, so an open panel is closed
        // first; LockOverlay hides the shell windows before the grab.
        function onLockRequested() {
            panelController.closePanel();
            lockOverlay.begin();
        }

        function onScreenshotRequested(clipboardOnly) {
            panelController.closePanel();
            areaPicker.open(clipboardOnly);
        }
    }

    // The power menu and the launcher request the lock through LockService
    // rather than the window manager's hotkey, so the panel they were opened
    // from has to be dismissed here too; LockOverlay hands the rest to begin().
    Connections {
        target: Lock

        function onLockRequested() {
            panelController.closePanel();
        }
    }

    Connections {
        target: Platform

        function onScreensChanged() {
            if (panelController.currentPanel === "")
                return;

            const screens = Platform.screens;
            for (let i = 0; i < screens.length; ++i) {
                if (screens[i].name === panelController.currentScreenName)
                    return;
            }
            panelController.closePanel();
        }
    }

    // Keep the AppBar above maximized and borderless applications. This is a
    // slow safety net; panel opening still raises the bars immediately.
    Timer {
        interval: 2000
        repeat: true
        running: true
        onTriggered: Platform.raiseAppBars()
    }

    FontLoader {
        id: bodyFont
        source: "qrc:/assets/fonts/GoogleSansFlex.ttf"
    }

    FontLoader {
        id: iconFont
        source: "qrc:/assets/fonts/MaterialSymbolsRounded.ttf"
    }

    FontLoader {
        id: rubikFont
        source: "qrc:/assets/fonts/Rubik-VariableFont_wght.ttf"
    }

    FontLoader {
        id: monoFont
        source: "qrc:/assets/fonts/CaskaydiaCoveNerdFont-Regular.ttf"
    }

    Instantiator {
        model: Platform.screens

        delegate: Window {
            id: barWindow

            required property var modelData

            screen: modelData
            x: modelData.geometry.x
            y: modelData.geometry.y
            width: 60
            // The monitor, not the work area: the frame guards reserve 10 px on
            // the top and bottom edges, and the bar must still span the whole
            // screen or the shell's frame stops short at the corners.
            height: modelData.geometry.height
            // The picker covers the whole monitor while a selection is being
            // made, and the bar would fight it for the topmost slot. The
            // frozen frame already shows the bar, so hiding it is invisible.
            // The lock screen hides it for the same reason: an AppBar raised
            // by the shell timer must never cover the lock window. A fullscreen
            // application hides it too: the shell steps aside for the video
            // instead of drawing a bar over it.
            visible: !areaPicker.active && !lockOverlay.active && !lockOverlay.preparing
                && WindowManager.fullscreenScreens.indexOf(modelData.name) < 0
            visibility: Window.Windowed
            flags: Qt.FramelessWindowHint | Qt.Tool | Qt.WindowStaysOnTopHint
            color: "transparent"
            title: "Caelestia Shell — Windows"

            Component.onCompleted: Platform.registerLeftAppBar(barWindow, 60)
            Component.onDestruction: Platform.unregisterAppBar(barWindow)

            // Keyboard shortcuts live on the overlay window (F8/F9/F10/F1/
            // Ctrl+Q/Esc). Duplicating them here made Qt report ambiguous
            // shortcuts, so the bar window keeps none.

            RefreshDriver {
                targetWindow: barWindow
                active: app.smoothRefresh
            }

            Bar {
                anchors.fill: parent
                theme: appTheme
                panelController: panelController
                screenName: modelData.name
                screenY: barWindow.y
                z: 1
            }
        }
    }

    Instantiator {
        model: Platform.screens

        delegate: OverlayWindow {
            required property var modelData

            screenName: modelData.name
            theme: appTheme
            controller: panelController
            suspended: lockOverlay.preparing || lockOverlay.active
                || WindowManager.fullscreenScreens.indexOf(modelData.name) >= 0
            smoothRefresh: app.smoothRefresh
        }
    }

    // Application dock. It occupies the bottom band of the screen but stays
    // collapsed against the edge until the pointer touches it, like Caelestia's
    // bar wrapper: nothing is reserved, the surface floats over the windows
    // while it is shown.
    Instantiator {
        model: Platform.screens

        delegate: Window {
            id: dockWindow

            required property var modelData

            screen: modelData
            x: modelData.geometry.x
            y: modelData.geometry.y + modelData.geometry.height - height
            width: modelData.geometry.width
            // Proportions, not a fixed band: 128 px is right on a 1080p screen
            // and eats a sixth of a 768p laptop. Never below the pill plus its
            // margins (96 px).
            readonly property int dockBand: Math.max(96,
                Math.min(128, Math.round(modelData.geometry.height * 0.115)))
            height: dockBand
            visible: !areaPicker.active && !lockOverlay.active && !lockOverlay.preparing
                && WindowManager.fullscreenScreens.indexOf(modelData.name) < 0
            visibility: Window.Windowed
            flags: Qt.FramelessWindowHint | Qt.Tool | Qt.WindowStaysOnTopHint
            color: "transparent"
            title: "Caelestia Dock — Windows"

            Dock {
                anchors.fill: parent
                theme: appTheme
                screenGeometry: dockWindow.modelData.geometry
            }
        }
    }

    // The shell draws a 10 px frame around the wallpaper hole, and native
    // maximised windows know nothing about it: they slid under the frame (and
    // under the dock band). Thin app bars reserve the three edges the left bar
    // does not own, so maximise and snap stay native — they simply stop inside
    // the frame. Click-through, so the frame keeps behaving like glass.
    Instantiator {
        model: Platform.screens

        delegate: Item {
            id: guardHost

            required property var modelData

            Window {
                id: topGuard

                screen: guardHost.modelData
                x: guardHost.modelData.geometry.x
                y: guardHost.modelData.geometry.y
                width: guardHost.modelData.geometry.width
                height: 10
                visibility: Window.Windowed
                flags: Qt.FramelessWindowHint | Qt.Tool | Qt.WindowStaysOnTopHint
                color: "transparent"
                title: "Caelestia Frame Top"

                Component.onCompleted: {
                    Platform.registerTopAppBar(topGuard, 10);
                    Platform.setWindowInputState(topGuard, true, false);
                }
                Component.onDestruction: Platform.unregisterAppBar(topGuard)
            }

            Window {
                id: rightGuard

                screen: guardHost.modelData
                x: guardHost.modelData.geometry.x + guardHost.modelData.geometry.width - 10
                y: guardHost.modelData.geometry.y
                width: 10
                height: guardHost.modelData.geometry.height
                visibility: Window.Windowed
                flags: Qt.FramelessWindowHint | Qt.Tool | Qt.WindowStaysOnTopHint
                color: "transparent"
                title: "Caelestia Frame Right"

                Component.onCompleted: {
                    Platform.registerRightAppBar(rightGuard, 10);
                    Platform.setWindowInputState(rightGuard, true, false);
                }
                Component.onDestruction: Platform.unregisterAppBar(rightGuard)
            }

            Window {
                id: bottomGuard

                screen: guardHost.modelData
                x: guardHost.modelData.geometry.x
                y: guardHost.modelData.geometry.y + guardHost.modelData.geometry.height - 10
                width: guardHost.modelData.geometry.width
                height: 10
                visibility: Window.Windowed
                flags: Qt.FramelessWindowHint | Qt.Tool | Qt.WindowStaysOnTopHint
                color: "transparent"
                title: "Caelestia Frame Bottom"

                Component.onCompleted: {
                    Platform.registerBottomAppBar(bottomGuard, 10);
                    Platform.setWindowInputState(bottomGuard, true, false);
                }
                Component.onDestruction: Platform.unregisterAppBar(bottomGuard)
            }
        }
    }

    // Screenshot area selection (Ctrl+Alt+S saves, Ctrl+Alt+X copies).
    AreaPicker {
        id: areaPicker

        theme: appTheme
    }
}
