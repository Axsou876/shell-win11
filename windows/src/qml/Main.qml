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

    Theme {
        id: appTheme

        dark: Platform.darkMode
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
            height: modelData.availableGeometry.height
            // The picker covers the whole monitor while a selection is being
            // made, and the bar would fight it for the topmost slot. The
            // frozen frame already shows the bar, so hiding it is invisible.
            // The lock screen hides it for the same reason: an AppBar raised
            // by the shell timer must never cover the lock window.
            visible: !areaPicker.active && !lockOverlay.active && !lockOverlay.preparing
            visibility: Window.Windowed
            flags: Qt.FramelessWindowHint | Qt.Tool | Qt.WindowStaysOnTopHint
            color: "transparent"
            title: "Caelestia Shell — Windows"

            Component.onCompleted: Platform.registerLeftAppBar(barWindow, 60)
            Component.onDestruction: Platform.unregisterAppBar(barWindow)

            // Keyboard shortcuts live on the overlay window (F8/F9/F10/F1/
            // Ctrl+Q/Esc). Duplicating them here made Qt report ambiguous
            // shortcuts, so the bar window keeps none.

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
        }
    }

    // Screenshot area selection (Ctrl+Alt+S saves, Ctrl+Alt+X copies).
    AreaPicker {
        id: areaPicker

        theme: appTheme
    }
}
