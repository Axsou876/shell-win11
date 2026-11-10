import QtQuick
import Caelestia.Platform 1.0

// One always-visible, click-through surface per monitor. It covers the full
// monitor width and the Windows work area height, so the bar and the frame
// are drawn as one composition. The AppBar windows stay above it and keep
// receiving clicks; WS_EX_TRANSPARENT lets every other click reach the app
// behind while no panel is open.
Window {
    id: overlay

    required property string screenName
    required property var theme
    required property var controller

    readonly property var screenData: {
        const screens = Platform.screens;
        for (let i = 0; i < screens.length; ++i) {
            if (screens[i] && screens[i].name === screenName)
                return screens[i];
        }
        return null;
    }
    readonly property rect screenRect: screenData ? screenData.geometry : Qt.rect(0, 0, 0, 0)
    readonly property int workHeight: screenData ? screenData.availableGeometry.height : 0
    readonly property bool interactive: controller.currentPanel !== ""
        && controller.currentScreenName === screenName
    property bool ready: false

    title: "Caelestia Panel Overlay"
    screen: screenData
    x: screenRect.x
    y: screenRect.y
    width: screenRect.width
    height: workHeight
    // Keep the window hidden until the no-activate style is applied, so the
    // always-visible frame never steals focus from the user's app at startup.
    visibility: overlay.ready ? Window.Windowed : Window.Hidden
    flags: Qt.FramelessWindowHint | Qt.Tool | Qt.WindowStaysOnTopHint
    color: "transparent"

    function syncInput() {
        const hover = overlay.controller.openReason === "hover";
        // Hover panels stay NOACTIVATE so they never steal keyboard focus
        // from the user's app. Mouse clicks still reach the panel; explicit
        // opens (click/F9/F10) remain focusable for keyboard input.
        Platform.setWindowInputState(overlay, !overlay.interactive,
            overlay.interactive && !hover);
        Platform.setEscapeHotkey(overlay, overlay.interactive);
        if (overlay.interactive) {
            Platform.raiseAppBars();
            if (!hover)
                Qt.callLater(() => overlay.requestActivate());
        }
    }

    // The original Dashboard trigger is the top 10 px strip above the
    // centered dashboard (not the whole screen edge). The closed overlay is
    // click-through, so raw mouse input from Platform feeds this check.
    function checkDashboardHover(pos) {
        if (!screenData)
            return;

        const localX = pos.x - screenRect.x;
        const localY = pos.y - screenRect.y;
        const inZone = localY >= 0 && localY <= overlay.theme.borderThickness
            && localX >= surface.dashboardX - overlay.theme.borderRounding
            && localX <= surface.dashboardX + surface.dashboardW + overlay.theme.borderRounding;

        if (overlay.controller.currentPanel === "") {
            if (inZone) {
                overlay.controller.hoverTrigger("dashboard", overlay.screenName, 0);
            } else if (overlay.controller.hoverPanel === "dashboard"
                    && overlay.controller.hoverScreenName === overlay.screenName) {
                overlay.controller.cancelHover();
            }
        }
    }

    Component.onCompleted: Qt.callLater(() => {
        overlay.syncInput();
        Platform.raiseAppBars();
        overlay.ready = true;
    })

    onInteractiveChanged: syncInput()

    // Activating a topmost overlay can raise it above the AppBar. Put the bars
    // back on top after every activation; the overlay must stay below them.
    onActiveChanged: {
        if (active)
            Qt.callLater(() => Platform.raiseAppBars());
    }

    // Applying the input styles before the first expose can race with Qt's
    // native window creation. Re-apply once the window is actually shown.
    onVisibilityChanged: {
        if (overlay.visibility === Window.Windowed)
            Qt.callLater(() => overlay.syncInput());
    }

    // Activating a topmost overlay can put it above the AppBar. Keep the bars
    // above for as long as the overlay owns the keyboard while a panel is open.
    Timer {
        interval: 150
        repeat: true
        running: overlay.interactive && overlay.active
        onTriggered: Platform.raiseAppBars()
    }

    Shortcut {
        sequence: "Esc"
        enabled: overlay.interactive
        onActivated: overlay.controller.closePanel()
    }

    // Temporary panel-preview triggers while the overlay owns the keyboard.
    // The bar keeps its own copies for the closed/bar-focused state.
    Shortcut {
        sequence: "F9"
        enabled: overlay.interactive
        onActivated: overlay.controller.togglePanel("sidebar", overlay.screenName, 0)
    }

    Shortcut {
        sequence: "F10"
        enabled: overlay.interactive
        onActivated: overlay.controller.togglePanel("dashboard", overlay.screenName, 0)
    }

    Shortcut {
        sequence: "F8"
        enabled: overlay.interactive
        onActivated: overlay.theme.dark = !overlay.theme.dark
    }

    Connections {
        target: overlay.controller

        function onPanelOpened() {
            overlay.syncInput();
        }

        function onPanelClosed() {
            overlay.syncInput();
        }
    }

    Connections {
        target: Platform

        function onGlobalMouseMoved(position) {
            overlay.checkDashboardHover(position);
        }

        function onEscapePressed() {
            if (overlay.controller.currentPanel !== ""
                    && overlay.controller.currentScreenName === overlay.screenName)
                overlay.controller.closePanel();
        }
    }

    ScreenSurface {
        id: surface

        anchors.fill: parent
        theme: overlay.theme
        controller: overlay.controller
        screenName: overlay.screenName
        popoutRect: Qt.rect(host.frameX, host.frameY, host.frameWidth, host.frameHeight)
        popoutVisible: host.frameVisible
    }

    // A 1/255 alpha pixel keeps the layered window hit-testable while staying
    // visually indistinguishable from a fully transparent backdrop.
    Rectangle {
        anchors.fill: parent
        color: "#01000000"
    }

    // Backdrop: consumes the outside click, exactly like the original
    // full-screen interaction layer.
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
        onClicked: overlay.controller.closePanel()
    }

    SidebarShell {
        x: surface.sidebarX
        y: surface.sidebarY
        width: surface.sidebarW
        height: surface.sidebarHeight
        opacity: surface.sidebarOpacity
        visible: surface.sidebarVisible
        deformScaleX: surface.sidebarScaleX
        deformScaleY: surface.sidebarScaleY
        theme: overlay.theme
    }

    DashboardShell {
        x: surface.dashboardX
        y: surface.dashboardY
        width: surface.dashboardW
        height: overlay.theme.dashboardHeight
        opacity: surface.dashboardOpacity
        visible: surface.dashboardVisible
        deformScaleX: surface.dashboardScaleX
        deformScaleY: surface.dashboardScaleY
        theme: overlay.theme

        onHoverActiveChanged: {
            if (overlay.controller.currentPanel === "dashboard")
                overlay.controller.panelHover(hoverActive);
        }
    }

    PanelHost {
        id: host

        anchors.fill: parent
        theme: overlay.theme
        controller: overlay.controller
        screenName: overlay.screenName
        overlayY: overlay.y
        deformScaleX: surface.popoutScaleX
        deformScaleY: surface.popoutScaleY
        deformOriginX: surface.popoutDeformOriginX

        onHoverActiveChanged: {
            const panel = overlay.controller.currentPanel;
            if (host.frameVisible && panel !== "" && panel !== "dashboard" && panel !== "sidebar")
                overlay.controller.panelHover(hoverActive);
        }
    }

    onScreenDataChanged: Qt.callLater(() => {
        overlay.syncInput();
        Platform.raiseAppBars();
    })
}


