import QtQuick
import ".."
import "../components"

Item {
    id: root

    required property var theme

    implicitWidth: icon.implicitHeight + theme.paddingSmall
    implicitHeight: icon.implicitHeight
    scale: powerArea.pressed ? 0.9 : 1

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
        onClicked: console.log("Caelestia power button clicked")
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

