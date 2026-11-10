import QtQuick
import Caelestia.Platform 1.0
import "../components"

// qmllint disable unqualified

Rectangle {
    id: root

    required property var theme
    required property var panelController
    required property string screenName
    required property real screenY

    implicitWidth: theme.barInnerWidth
    implicitHeight: layout.implicitHeight + theme.paddingMedium * 2
    radius: width / 2
    color: theme.surfaceContainer

    function networkIcon() {
        if (!Network.known)
            return "wifi_off";
        if (Network.connectionType === "ethernet")
            return "cable";
        if (Network.connectionType === "wifi") {
            const icons = ["signal_wifi_0_bar", "network_wifi_1_bar", "network_wifi_2_bar",
                "network_wifi_3_bar", "network_wifi"];
            const level = Math.max(0, Math.min(4, Math.floor(Network.signalStrength / 20)));
            return icons[level];
        }
        return "wifi_off";
    }

    function batteryIcon() {
        if (!Battery.known || Battery.level < 0)
            return "battery_unknown";
        if (Battery.level >= 100)
            return Battery.charging ? "battery_charging_full" : "battery_full";
        let level = Math.floor(Battery.level / 100 * 7);
        if (Battery.charging && (level === 4 || level === 1))
            level--;
        return Battery.charging ? "battery_charging_" + (level + 3) * 10 : "battery_" + level + "_bar";
    }

    function audioIcon() {
        if (Audio.muted)
            return "no_sound";
        if (Audio.volume >= 50)
            return "volume_up";
        if (Audio.volume > 0)
            return "volume_down";
        return "volume_mute";
    }

    function iconAt(px, py) {
        const p = layout.mapFromItem(root, px, py);
        if (p.y < 0 || p.y > layout.height)
            return null;
        return layout.childAt(layout.width / 2, p.y);
    }

    function triggerAt(px, py, reason) {
        const icon = iconAt(px, py);
        if (!icon || !icon.visible || !icon.panelName)
            return false;

        const centre = icon.mapToItem(null, 0, icon.height / 2);
        const anchor = root.screenY + centre.y;
        if (reason === "hover")
            root.panelController.hoverTrigger(icon.panelName, root.screenName, anchor);
        else
            root.panelController.openPanel(icon.panelName, root.screenName, anchor, "explicit");
        return true;
    }

    Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: root.theme.onSurfaceColour
        opacity: openArea.containsMouse ? 0.04 : 0

        Behavior on opacity {
            NumberAnimation {
                duration: root.theme.durationFastEffects
            }
        }
    }

    Column {
        id: layout

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.theme.paddingMedium
        spacing: root.theme.spacingMedium / 2

        MaterialIcon {
            anchors.horizontalCenter: parent.horizontalCenter
            theme: root.theme
            text: root.audioIcon()
            color: root.theme.secondary
            iconSize: root.theme.statusIconSize
            visible: Audio.present
            opacity: Audio.known ? 1 : 0.45
            property string panelName: "audio"
        }

        MaterialIcon {
            anchors.horizontalCenter: parent.horizontalCenter
            theme: root.theme
            text: root.networkIcon()
            color: root.theme.secondary
            iconSize: root.theme.statusIconSize
            opacity: Network.known ? 1 : 0.45
            property string panelName: "network"
        }

        MaterialIcon {
            anchors.horizontalCenter: parent.horizontalCenter
            theme: root.theme
            text: Bluetooth.enabled ? "bluetooth" : "bluetooth_disabled"
            color: root.theme.secondary
            iconSize: root.theme.statusIconSize
            visible: Bluetooth.present
            opacity: Bluetooth.known ? 1 : 0.45
            property string panelName: "bluetooth"
        }

        MaterialIcon {
            anchors.horizontalCenter: parent.horizontalCenter
            theme: root.theme
            text: root.batteryIcon()
            color: !Battery.onAc && Battery.level >= 0 && Battery.level <= 20
                ? root.theme.error
                : root.theme.secondary
            iconSize: root.theme.statusIconSize
            fill: 1
            visible: Battery.present
            opacity: Battery.known ? 1 : 0.45
            property string panelName: "battery"
        }
    }

    // Hover follows the real status icon under the pointer. A click opens the
    // same panel explicitly and gives it keyboard focus.
    MouseArea {
        id: openArea

        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor

        onPositionChanged: mouse => root.triggerAt(mouse.x, mouse.y, "hover")
        onExited: root.panelController.hoverBarExit(root.screenName)
        onClicked: mouse => root.triggerAt(mouse.x, mouse.y, "explicit")
    }
}

