pragma ComponentBehavior: Bound

import QtQuick
import Caelestia.Platform 1.0
import ".."

// Coordinator for the session lock. The desktop must be frozen before any
// lock window exists, so opening happens in two steps: capture every monitor
// into ScreenCaptureService, then instantiate one LockWindow per screen.
// Windows stay mapped for a short exit animation after a successful unlock.
Item {
    id: root

    required property var theme

    // Captured monitors and their provider token, keyed by screen name. A
    // monitor whose capture failed still gets a window (the wallpaper is used
    // instead), so no desktop can stay visible next to a locked one.
    property var screens: []
    property var tokens: ({})

    property bool active: false
    property bool closing: false
    // True between the lock request and the capture. The panel overlays and
    // the bar hide on it, so an open sidebar/popout cannot end up frozen into
    // the lock background.
    property bool preparing: false

    function tokenFor(name) {
        const token = root.tokens[name];
        return typeof token === "number" ? token : 0;
    }

    // Called by the hotkey, the power menu and the launcher. Hiding the shell
    // surfaces is asynchronous at the native window level, so the capture
    // waits one short tick after they are told to hide.
    function begin() {
        if (root.active || root.preparing)
            return;
        root.preparing = true;
        prepareTimer.start();
    }

    function open() {
        if (root.active)
            return;

        const monitors = Platform.screens;
        if (monitors.length === 0)
            return;

        const tokens = ({});
        for (let i = 0; i < monitors.length; ++i) {
            const screen = monitors[i];
            if (!screen)
                continue;
            const token = Screenshots.captureScreen(screen);
            if (token > 0)
                tokens[screen.name] = token;
        }

        root.tokens = tokens;
        root.screens = monitors;
        root.closing = false;
        root.preparing = false;
        root.active = true;
        // Only now, with the frames frozen, may a lock window appear.
        Lock.show();
    }

    Timer {
        id: prepareTimer

        interval: 60
        onTriggered: root.open()
    }

    function finish() {
        if (!root.active)
            return;
        root.closing = true;
        closeTimer.start();
    }

    function restore() {
        closeTimer.stop();
        root.active = false;
        root.closing = false;
        root.preparing = false;
        root.screens = [];
        root.tokens = {};
        // The frozen frames are gone with the windows.
        Screenshots.clear();
        // The lock window owned the keyboard; hand it back to the application
        // that had it before the screen was locked.
        Qt.callLater(() => Platform.restoreLastExternalWindow());
    }

    // The Windows secure desktop is the authoritative lock. Leave the overlay
    // first, so unlocking Windows never lands back on a still-mapped screen.
    function useWindowsLock() {
        Lock.unlock();
        Qt.callLater(() => {
            if (!Lock.lockWindows()) {
                Notification.addNotification("Caelestia", "Verrouillage",
                    I18n.t("Windows a refusé le verrouillage de la session."));
            }
        });
    }

    Timer {
        id: closeTimer

        interval: 280
        onTriggered: root.restore()
    }

    Connections {
        target: Lock

        function onLockRequested() {
            // Same two-step sequence as the global hotkey: hide the shell
            // surfaces, then freeze the desktop. Capturing straight away would
            // bake an open panel into the locked background.
            root.begin();
        }

        function onUnlocked() {
            root.finish();
        }
    }

    Instantiator {
        model: root.active ? root.screens : []

        delegate: LockWindow {
            required property var modelData

            theme: root.theme
            overlay: root
            screenData: modelData
            token: root.tokenFor(modelData.name)
            closing: root.closing
        }
    }
}
