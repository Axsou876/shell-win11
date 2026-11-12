import QtQuick
import Caelestia.Platform 1.0

// Single active panel plus the central hover state machine. Hover delays are
// an explicit Windows adaptation: the Linux shell reacts immediately; these
// short delays avoid accidental openings and flicker between icons.
QtObject {
    id: root

    property string currentPanel: ""
    property string currentScreenName: ""
    property string openReason: "" // "hover" or "explicit"
    property real anchorY: 0
    property bool closing: false
    property int closeDuration: 500

    property int hoverOpenDelay: 120
    property int hoverCloseDelay: 220

    property string hoverPanel: ""
    property string hoverScreenName: ""
    property real hoverAnchorY: 0
    property bool pointerInPanel: false

    readonly property bool open: currentPanel !== ""

    signal panelOpened(string name)
    signal panelClosed()

    function openPanel(name, screenName, screenY, reason) {
        if (!name || !screenName)
            return;

        const requestedReason = reason === "hover" ? "hover" : "explicit";

        hoverOpenTimer.stop();
        hoverCloseTimer.stop();
        hoverPanel = "";
        closeTimer.stop();
        closing = false;
        anchorY = screenY;

        if (currentPanel === name && currentScreenName === screenName) {
            if (openReason !== requestedReason) {
                openReason = requestedReason;
                panelOpened(name);
            }
            return;
        }

        // Set the reason before the screen: a cross-screen switch triggers
        // interactiveChanged, whose syncInput must already know this is a
        // hover (otherwise it would make the overlay focusable and activate).
        openReason = requestedReason;
        currentScreenName = screenName;
        currentPanel = name;
        panelOpened(name);
    }

    function closePanel() {
        if (!currentPanel || closing)
            return;

        hoverOpenTimer.stop();
        hoverCloseTimer.stop();
        hoverPanel = "";
        pointerInPanel = false;
        closing = true;
        closeTimer.restart();
    }

    function togglePanel(name, screenName, screenY) {
        if (currentPanel === name && currentScreenName === screenName && !closing)
            closePanel();
        else
            openPanel(name, screenName, screenY, "explicit");
    }

    function finishClose() {
        if (!currentPanel) {
            closing = false;
            return;
        }

        currentPanel = "";
        currentScreenName = "";
        openReason = "";
        pointerInPanel = false;
        closing = false;
        panelClosed();
        Qt.callLater(() => Platform.restoreLastExternalWindow());
    }

    // Called repeatedly while the pointer is over a bar icon. The timer is
    // not restarted for the same target, so a resting hover commits after the
    // intent delay while a fast sweep only commits for the final icon.
    function hoverTrigger(name, screenName, screenY) {
        if (!name || !screenName)
            return;

        hoverCloseTimer.stop();

        if (currentPanel === name && currentScreenName === screenName
                && openReason === "hover") {
            hoverOpenTimer.stop();
            hoverPanel = "";
            return;
        }

        if (hoverPanel === name && hoverScreenName === screenName)
            return;

        hoverPanel = name;
        hoverScreenName = screenName;
        hoverAnchorY = screenY;
        hoverOpenTimer.restart();
    }

    function commitHover() {
        if (!hoverPanel)
            return;

        const name = hoverPanel;
        const screen = hoverScreenName;
        const y = hoverAnchorY;
        hoverPanel = "";
        openPanel(name, screen, y, "hover");
    }

    function cancelHover() {
        hoverOpenTimer.stop();
        hoverPanel = "";
    }

    // The pointer is outside the dashboard trigger while no panel is open.
    // Cancel a pending bar hover so leaving before the intent delay does not
    // open a panel behind the pointer.
    function cancelPendingBarHover(screenName) {
        if (hoverPanel !== "" && hoverPanel !== "dashboard"
                && hoverScreenName === screenName)
            cancelHover();
    }

    // Called by the raw-input halo. Entering cancels an in-flight close;
    // leaving starts the grace delay.
    function panelHover(entered) {
        pointerInPanel = entered;

        if (entered) {
            hoverCloseTimer.stop();
        } else if (currentPanel !== "" && openReason === "hover"
                && hoverPanel === "") {
            hoverCloseTimer.restart();
        }
    }

    function commitHoverClose() {
        if (currentPanel !== "" && openReason === "hover"
                && !pointerInPanel && hoverPanel === "")
            closePanel();
    }

    property Timer closeTimer: Timer {
        interval: root.closeDuration
        onTriggered: root.finishClose()
    }

    property Timer hoverOpenTimer: Timer {
        interval: root.hoverOpenDelay
        onTriggered: root.commitHover()
    }

    property Timer hoverCloseTimer: Timer {
        interval: root.hoverCloseDelay
        onTriggered: root.commitHoverClose()
    }
}
