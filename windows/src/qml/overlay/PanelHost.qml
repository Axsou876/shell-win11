pragma ComponentBehavior: Bound

import QtQuick
import ".."
import "../popouts"

Item {
    id: host

    required property var theme
    required property var controller
    required property string screenName
    required property real overlayY

    property real deformScaleX: 1
    property real deformScaleY: 1
    property real deformOriginX: frame.width / 2

    property string loadedPanel: ""

    readonly property bool shown: controller.currentPanel !== ""
        && controller.currentScreenName === screenName
        && controller.currentPanel !== "sidebar"
        && controller.currentPanel !== "dashboard"
        && !controller.closing

    readonly property var panelItem: loader.item
    readonly property real panelWidth: panelItem && panelItem.implicitWidth > 0 ? panelItem.implicitWidth : 320
    readonly property real panelHeight: panelItem ? panelItem.implicitHeight : 0

    // Geometry of the blob panel drawn by ScreenSurface. The frame's width
    // also drives the surface reveal, so the content and the blob stay locked.
    readonly property real frameX: frame.x
    readonly property real frameY: frame.y
    readonly property real frameWidth: frame.width
    readonly property real frameHeight: frame.height
    readonly property bool frameVisible: frame.width > 0.5
    readonly property bool hoverActive: frame.hoverActive

    function sync() {
        if (shown) {
            unloadTimer.stop();
            if (loadedPanel !== controller.currentPanel)
                loadedPanel = controller.currentPanel;
        } else if (loadedPanel !== "") {
            unloadTimer.restart();
        }
    }

    Component.onCompleted: sync()
    onShownChanged: sync()

    Connections {
        target: host.controller

        function onCurrentPanelChanged() {
            host.sync();
        }

        function onClosingChanged() {
            host.sync();
        }
    }

    Timer {
        id: unloadTimer

        interval: 650
        onTriggered: {
            if (host.controller.currentPanel === "")
                host.loadedPanel = "";
        }
    }

    PopoutFrame {
        id: frame

        theme: host.theme
        opened: host.shown
        contentWidth: host.panelWidth
        contentHeight: host.panelHeight
        deformScaleX: host.deformScaleX
        deformScaleY: host.deformScaleY
        deformOriginX: host.deformOriginX
        x: host.theme.barRailWidth
        y: {
            const margin = host.theme.paddingLarge;
            const anchor = host.controller.anchorY - host.overlayY;
            return Math.max(margin, Math.min(anchor - frame.targetHeight / 2, host.height - frame.targetHeight - margin));
        }

        Behavior on y {
            Anim {
                type: Anim.DefaultSpatial
            }
        }
    }

    Loader {
        id: loader

        parent: frame.contentParent
        anchors.fill: parent
        active: host.loadedPanel !== ""
        sourceComponent: host.loadedPanel !== "" ? foundationComponent : null
    }

    Component {
        id: foundationComponent

        FoundationProbe {
            panelName: host.loadedPanel
            theme: host.theme
            controller: host.controller
        }
    }
}
