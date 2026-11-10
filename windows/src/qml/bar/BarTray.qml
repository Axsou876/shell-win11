import QtQuick
import Caelestia.Platform 1.0
import ".."
import "../components"

// qmllint disable unqualified

Item {
    id: root

    required property var theme

    implicitWidth: theme.barInnerWidth
    implicitHeight: theme.trayIconSize + theme.paddingExtraSmall * 2
    visible: Tray.available
    scale: trayMouse.pressed ? 0.92 : 1

    Behavior on scale {
        Anim {
            type: Anim.FastSpatial
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: height / 2
        color: root.theme.onSurfaceColour
        opacity: trayMouse.containsMouse ? 0.08 : 0

        Behavior on opacity {
            Anim {
                type: Anim.FastEffects
            }
        }
    }

    Logo {
        anchors.centerIn: parent
        width: root.theme.trayIconSize
        height: root.theme.trayIconSize
        theme: root.theme
    }

    MouseArea {
        id: trayMouse

        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        cursorShape: Qt.PointingHandCursor
        onClicked: Tray.showMenu()
    }
}
