pragma ComponentBehavior: Bound

import QtQuick
import Caelestia.Platform 1.0
import "../components"

// Windows port of the Linux area picker. The desktop is frozen once, before
// any picker window exists, then one full-monitor overlay per captured screen
// lets the user drag a region (or click a window to select it). The freeze is
// what keeps the overlay out of its own screenshot.
Item {
    id: root

    required property var theme

    property bool active: false
    property bool clipboardOnly: false
    // Captured screens and their provider tokens. Only screens captured
    // successfully get an overlay.
    property var screens: []
    property var tokens: ({})

    function tokenFor(screenName): int {
        const token = root.tokens[screenName];
        return typeof token === "number" ? token : 0;
    }

    function open(clipboardOnlyMode): void {
        if (root.active)
            return;

        const captured = [];
        const tokens = ({});

        // Capture first: showing a picker window before the grab would put
        // the overlay into the frozen frame.
        const monitors = Platform.screens;
        for (let i = 0; i < monitors.length; ++i) {
            const screen = monitors[i];
            if (!screen)
                continue;
            const token = Screenshots.captureScreen(screen);
            if (token <= 0)
                continue;
            captured.push(screen);
            tokens[screen.name] = token;
        }

        if (captured.length === 0) {
            Notification.addNotification("Caelestia", "Capture d'écran",
                Screenshots.error || "Aucun écran disponible.");
            Screenshots.clear();
            return;
        }

        root.clipboardOnly = clipboardOnlyMode === true;
        root.tokens = tokens;
        root.screens = captured;
        root.active = true;
    }

    function complete(token, x, y, width, height, viewWidth, viewHeight): void {
        if (width < 1 || height < 1) {
            root.cancel();
            return;
        }

        if (root.clipboardOnly) {
            if (!Screenshots.copyRegion(token, x, y, width, height, viewWidth,
                    viewHeight)) {
                Notification.addNotification("Caelestia", "Capture d'écran",
                    Screenshots.error || "Copie impossible.");
            } else {
                Notification.addNotification("Caelestia", "Capture d'écran",
                    "Zone copiée dans le presse-papiers.");
            }
        } else {
            const path = Screenshots.saveRegion(token, x, y, width, height,
                viewWidth, viewHeight);
            if (path.length === 0) {
                Notification.addNotification("Caelestia", "Capture d'écran",
                    Screenshots.error || "Enregistrement impossible.");
            } else {
                Notification.addNotification("Caelestia", "Capture d'écran",
                    "Enregistrée dans " + path);
            }
        }

        root.cancel();
    }

    function cancel(): void {
        if (!root.active)
            return;
        root.active = false;
        root.screens = [];
        root.tokens = ({});

        // The overlays are gone; the frozen frames are no longer needed.
        Screenshots.clear();

        // Without this the window manager activates whatever is next in the
        // z-order (often the launch console) instead of the user's app.
        Qt.callLater(() => Platform.restoreLastExternalWindow());
    }

    // Escape is also registered as a global hotkey by the first overlay, so a
    // picker opened while another window keeps focus can still be cancelled.
    Connections {
        target: Platform

        function onEscapePressed(): void {
            if (root.active)
                root.cancel();
        }
    }

    Instantiator {
        model: root.active ? root.screens : []

        delegate: Window {
            id: picker

            required property var modelData
            required property int index

            readonly property int token: root.tokenFor(modelData.name)
            // Selection in QML window coordinates, normalised so a drag in
            // any direction gives a positive rectangle.
            readonly property real selX: Math.max(0, Math.min(sx, ex))
            readonly property real selY: Math.max(0, Math.min(sy, ey))
            readonly property real selRight: Math.min(width, Math.max(sx, ex))
            readonly property real selBottom: Math.min(height, Math.max(sy, ey))
            readonly property real selWidth: Math.max(0, selRight - selX)
            readonly property real selHeight: Math.max(0, selBottom - selY)

            property real sx: 0
            property real sy: 0
            property real ex: 0
            property real ey: 0

            // Geometry comes from Platform.fitWindowToScreen rather than QML
            // x/y/width/height: tool windows are created on the primary
            // screen, and QML geometry bindings would keep the primary DPI.
            visibility: Window.Hidden
            flags: Qt.FramelessWindowHint | Qt.Tool | Qt.WindowStaysOnTopHint
            color: "transparent"
            title: "Caelestia — sélection de zone"

            function resetSelection(): void {
                // Covers most of the screen with the picker's own size, so it
                // runs after fitWindowToScreen has possibly resized the window.
                sx = width / 2 - 100;
                sy = height / 2 - 100;
                ex = width / 2 + 100;
                ey = height / 2 + 100;
            }

            function snapToWindow(px, py): bool {
                const hit = Screenshots.windowAt(token, Math.round(px),
                    Math.round(py), Math.round(width), Math.round(height));
                if (!hit || !hit.valid)
                    return false;
                sx = hit.x;
                sy = hit.y;
                ex = hit.x + hit.width;
                ey = hit.y + hit.height;
                return true;
            }

            Component.onCompleted: {
                // Undo the NOACTIVATE style the shell applies to its overlay
                // surfaces; this window must receive the mouse and Escape.
                Platform.setWindowInputState(picker, false, true);
                if (index === 0)
                    Platform.setEscapeHotkey(picker, true);
                Qt.callLater(() => {
                    Platform.fitWindowToScreen(picker, modelData);
                    picker.resetSelection();
                    picker.visibility = Window.Windowed;
                    picker.requestActivate();
                    surface.forceActiveFocus();
                });
            }

            Component.onDestruction: {
                if (index === 0)
                    Platform.setEscapeHotkey(picker, false);
            }

            Item {
                id: surface

                anchors.fill: parent
                focus: true

                Keys.onEscapePressed: root.cancel()

                Image {
                    anchors.fill: parent
                    source: picker.token > 0
                        ? "image://screencapture/" + picker.token
                        : ""
                    fillMode: Image.Stretch
                    cache: false
                    asynchronous: false
                    smooth: true
                }

                // Dim outside the selection, one rectangle per side. A mask
                // shader would be shorter, but this stays readable and has no
                // shader compilation cost at popup time.
                Rectangle {
                    x: 0
                    y: 0
                    width: surface.width
                    height: picker.selY
                    color: root.theme.secondaryContainer
                    opacity: 0.45
                }

                Rectangle {
                    x: 0
                    y: picker.selBottom
                    width: surface.width
                    height: Math.max(0, surface.height - picker.selBottom)
                    color: root.theme.secondaryContainer
                    opacity: 0.45
                }

                Rectangle {
                    x: 0
                    y: picker.selY
                    width: picker.selX
                    height: picker.selHeight
                    color: root.theme.secondaryContainer
                    opacity: 0.45
                }

                Rectangle {
                    x: picker.selRight
                    y: picker.selY
                    width: Math.max(0, surface.width - picker.selRight)
                    height: picker.selHeight
                    color: root.theme.secondaryContainer
                    opacity: 0.45
                }

                Rectangle {
                    x: picker.selX
                    y: picker.selY
                    width: picker.selWidth
                    height: picker.selHeight
                    color: "transparent"
                    border.width: 2
                    border.color: root.theme.primary
                }

                // Live size badge; flips above the selection when there is no
                // room underneath.
                Rectangle {
                    id: badge

                    x: Math.max(0, Math.min(picker.selX,
                        surface.width - badge.width - 8))
                    y: picker.selY + picker.selHeight + 8 + height > surface.height
                        ? Math.max(0, picker.selY - height - 8)
                        : picker.selY + picker.selHeight + 8
                    width: badgeText.implicitWidth + 24
                    height: badgeText.implicitHeight + 10
                    radius: root.theme.roundingSmall
                    color: root.theme.surfaceContainer
                    border.width: 1
                    border.color: root.theme.outlineVariant

                    StyledText {
                        id: badgeText

                        anchors.centerIn: parent
                        theme: root.theme
                        role: "labelSmall"
                        color: root.theme.onSurfaceColour
                        text: Math.round(picker.selWidth) + " × "
                            + Math.round(picker.selHeight)
                    }
                }

                MouseArea {
                    id: mouse

                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.CrossCursor
                    acceptedButtons: Qt.LeftButton | Qt.RightButton

                    property real pressX: 0
                    property real pressY: 0
                    property bool dragged: false

                    onPressed: event => {
                        if (event.button === Qt.RightButton) {
                            root.cancel();
                            return;
                        }
                        mouse.pressX = event.x;
                        mouse.pressY = event.y;
                        mouse.dragged = false;
                        // A click without a drag selects (and later saves) the
                        // window under the cursor.
                        picker.snapToWindow(event.x, event.y);
                    }

                    onPositionChanged: event => {
                        if (!mouse.pressed) {
                            picker.snapToWindow(event.x, event.y);
                            return;
                        }
                        if (!mouse.dragged
                            && Math.abs(event.x - mouse.pressX)
                                + Math.abs(event.y - mouse.pressY) < 3)
                            return;
                        mouse.dragged = true;
                        picker.sx = mouse.pressX;
                        picker.sy = mouse.pressY;
                        picker.ex = event.x;
                        picker.ey = event.y;
                    }

                    onReleased: event => {
                        if (event.button === Qt.RightButton)
                            return;
                        if (picker.selWidth < 1 || picker.selHeight < 1) {
                            root.cancel();
                            return;
                        }
                        root.complete(picker.token, Math.round(picker.selX),
                            Math.round(picker.selY),
                            Math.round(picker.selWidth),
                            Math.round(picker.selHeight),
                            Math.round(picker.width), Math.round(picker.height));
                    }
                }
            }
        }
    }
}

// qmllint disable unqualified
