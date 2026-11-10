import QtQuick
import ".."
import "../components"

Item {
    id: root

    required property var theme
    required property var panelController
    required property string screenName
    required property real screenY
    required property real screenHeight

    implicitWidth: icon.implicitHeight + theme.paddingSmall
    implicitHeight: icon.implicitHeight
    scale: powerArea.pressed ? 0.9 : powerArea.containsMouse ? 1.08 : 1

    Behavior on scale {
        Anim {
            type: Anim.FastSpatial
        }
    }

    StateLayer {
        id: powerArea

        anchors.centerIn: parent
        width: icon.implicitHeight + root.theme.paddingSmall
        height: width
        radius: width / 2
        theme: root.theme
        onClicked: root.panelController.togglePanel("power", root.screenName,
            root.screenY + root.screenHeight - 90)
    }

    MaterialIcon {
        id: icon

        anchors.centerIn: parent
        theme: root.theme
        text: "power_settings_new"
        color: root.theme.error
        iconSize: root.theme.iconSmall
        weight: Font.Bold
    }
}

