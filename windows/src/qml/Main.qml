pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Window
import Caelestia.Platform 1.0
import "overlay"

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
            visible: true
            visibility: Window.Windowed
            flags: Qt.FramelessWindowHint | Qt.Tool | Qt.WindowStaysOnTopHint
            color: "transparent"
            title: "Caelestia Shell — Windows Phase 2.6"

            Component.onCompleted: Platform.registerLeftAppBar(barWindow, 60)
            Component.onDestruction: Platform.unregisterAppBar(barWindow)
            Shortcut {
                sequence: "Ctrl+Q"
                onActivated: Qt.quit()
            }

            Shortcut {
                sequence: "Esc"
                enabled: panelController.currentPanel !== ""
                    && panelController.currentScreenName === modelData.name
                onActivated: panelController.closePanel()
            }

            // Temporary surface preview triggers. Step 2 replaces them with
            // the real bar entries (dashboard top-edge/bar target, sidebar
            // notification target).
            Shortcut {
                sequence: "F9"
                onActivated: panelController.togglePanel("sidebar", modelData.name, 0)
            }

            Shortcut {
                sequence: "F10"
                onActivated: panelController.togglePanel("dashboard", modelData.name, 0)
            }

            Shortcut {
                sequence: "F8"
                onActivated: appTheme.dark = !appTheme.dark
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
        }
    }
}
