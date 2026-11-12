pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import Caelestia.Platform 1.0
import "../components"

// One full-monitor lock surface. The frozen desktop capture is shown blurred
// behind a centred lock box that mirrors the Linux layout: weather, system
// fetch and media on the left, the clock and password field in the middle,
// resources and the notification dock on the right.
Window {
    id: root

    required property var theme
    required property var overlay
    required property var screenData
    required property int token
    property bool closing: false

    readonly property rect monitor: screenData ? screenData.geometry : Qt.rect(0, 0, 0, 0)
    // The Linux lock scales its content from the screen height; the Windows
    // desktop spans many DPI levels, so the factor is clamped to stay legible
    // on small laptops and not oversized on 4K monitors.
    readonly property real uiScale: Math.max(0.72, Math.min(1.1, monitor.height / 1080))
    readonly property real boxHeight: Math.min(height - 48 * uiScale, monitor.height * 0.7)
    readonly property real boxWidth: Math.min(width - 48 * uiScale, boxHeight * 16 / 9)
    readonly property real centerWidth: Math.min(600 * uiScale, boxWidth * 0.36)
    readonly property real columnWidth: (boxWidth - centerWidth
        - root.theme.paddingExtraLarge * 2 - root.theme.spacingExtraLarge * 1.5) / 2

    title: "Caelestia Lock Screen"
    screen: screenData
    visibility: Window.Hidden
    flags: Qt.FramelessWindowHint | Qt.Tool | Qt.WindowStaysOnTopHint
    color: "black"

    // The lock surface has no user-reachable close: WS_SYSMENU is stripped once
    // the window is mapped, so Alt+F4 has nothing to post, and a Qt.Tool window
    // never appears in the taskbar. Rejecting QCloseEvent in QML would be
    // simpler but also blocks QCoreApplication::quit() and Windows session end.
    function grabKeyboard(): void {
        root.requestActivate();
        keyboard.forceActiveFocus();
    }

    Component.onCompleted: {
        // Undo the NOACTIVATE style the shell applies to its overlay surfaces:
        // the lock window must own the mouse and the keyboard.
        Platform.setWindowInputState(root, false, true);
        Qt.callLater(() => {
            Platform.fitWindowToScreen(root, root.screenData);
            root.visibility = Window.Windowed;
            // After the show, or Qt re-applies the style it derives from the
            // window flags and puts the system menu back.
            Platform.disableWindowClose(root);
            root.grabKeyboard();
        });
    }

    FocusScope {
        id: keyboard

        anchors.fill: parent
        focus: true

        // --- Frozen desktop -------------------------------------------------
        Item {
            id: background

            anchors.fill: parent
            opacity: root.closing ? 0 : 1
            layer.enabled: true
            layer.effect: MultiEffect {
                blurEnabled: true
                blur: 1
                blurMax: 64
                autoPaddingEnabled: true
            }

            Behavior on opacity {
                Anim {
                    type: Anim.Standard
                }
            }

            Image {
                anchors.fill: parent
                // A captured monitor keeps the real desktop (and any window
                // that was open). Without a capture the Windows wallpaper is
                // the closest truthful stand-in.
                source: root.token > 0
                    ? "image://screencapture/" + root.token
                    : (Platform.wallpaperAvailable
                        ? "file:///" + Platform.wallpaperPath.replace(/\\/g, "/") : "")
                fillMode: Image.PreserveAspectCrop
                cache: false
                asynchronous: false
                smooth: true
            }
        }

        // Readability scrim over the blurred capture.
        Rectangle {
            anchors.fill: parent
            color: "#5c000000"
            opacity: root.closing ? 0 : 1

            Behavior on opacity {
                Anim {
                    type: Anim.Standard
                }
            }
        }

        // --- Lock box -------------------------------------------------------
        Item {
            id: box

            property bool entered: false

            anchors.centerIn: parent
            width: root.boxWidth
            height: root.boxHeight
            opacity: entered && !root.closing ? 1 : 0
            scale: entered && !root.closing ? 1 : 0.92
            rotation: entered ? 0 : 4

            Behavior on opacity {
                Anim {
                    type: Anim.DefaultEffects
                }
            }

            Behavior on scale {
                Anim {
                    type: Anim.FastSpatial
                }
            }

            Behavior on rotation {
                Anim {
                    type: Anim.FastSpatial
                }
            }

            Component.onCompleted: Qt.callLater(() => box.entered = true)

            Rectangle {
                anchors.fill: parent
                radius: root.theme.roundingExtraLarge * 1.5
                color: Qt.alpha(root.theme.surface, 0.94)
                border.width: 1
                border.color: Qt.alpha(root.theme.outlineVariant, 0.3)
                layer.enabled: true
                layer.effect: MultiEffect {
                    shadowEnabled: true
                    blurMax: 32
                    shadowOpacity: 0.6
                }
            }

            RowLayout {
                anchors.fill: parent
                anchors.margins: root.theme.paddingExtraLarge
                spacing: root.theme.spacingExtraLarge * 1.5

                // Left column: environment, at-a-glance system and media.
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.maximumWidth: root.columnWidth
                    spacing: root.theme.spacingMedium

                    LockWeather {
                        Layout.fillWidth: true
                        theme: root.theme
                        scaleFactor: root.uiScale
                    }

                    LockFetch {
                        Layout.fillWidth: true
                        theme: root.theme
                        scaleFactor: root.uiScale
                    }

                    LockMedia {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        theme: root.theme
                        scaleFactor: root.uiScale
                    }
                }

                // Centre column: clock, account and the one interactive input.
                LockCenter {
                    Layout.preferredWidth: root.centerWidth
                    Layout.maximumWidth: root.centerWidth
                    Layout.fillHeight: true
                    theme: root.theme
                    overlay: root.overlay
                    scaleFactor: root.uiScale
                }

                // Right column: resources and notifications.
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.maximumWidth: root.columnWidth
                    spacing: root.theme.spacingMedium

                    LockResources {
                        Layout.fillWidth: true
                        theme: root.theme
                        scaleFactor: root.uiScale
                    }

                    LockNotifDock {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        theme: root.theme
                        scaleFactor: root.uiScale
                    }
                }
            }
        }
    }
}
