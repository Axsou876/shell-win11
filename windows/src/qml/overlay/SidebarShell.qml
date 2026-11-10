import QtQuick
import "../components"

// Visual shell for the 430 px sidebar. The blob surface comes from
// ScreenSurface; this item is the inner notification dock. No shell
// notification service is connected yet.
Item {
    id: root

    required property var theme

    property real deformScaleX: 1
    property real deformScaleY: 1

    transform: Scale {
        xScale: root.deformScaleX
        yScale: root.deformScaleY
        origin.x: root.width / 2
        origin.y: root.height / 2
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {}
    }

    Rectangle {
        id: dock

        anchors.fill: parent
        anchors.margins: root.theme.paddingLarge
        radius: root.theme.roundingLarge
        color: root.theme.surfaceContainerLow

        Column {
            anchors.fill: parent
            anchors.margins: root.theme.paddingMedium
            spacing: root.theme.spacingMedium

            Item {
                width: parent.width
                height: 40

                Row {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: root.theme.spacingSmall

                    MaterialIcon {
                        anchors.verticalCenter: parent.verticalCenter
                        theme: root.theme
                        text: "notifications"
                        iconSize: 20
                        color: root.theme.onSurfaceVariantColour
                    }

                    StyledText {
                        anchors.verticalCenter: parent.verticalCenter
                        theme: root.theme
                        role: "titleSmall"
                        weight: Font.DemiBold
                        text: "Notifications"
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: root.theme.outlineVariant
                opacity: 0.6
            }

            Item {
                width: parent.width
                height: parent.height - 40 - 1 - root.theme.spacingMedium * 2

                Column {
                    anchors.centerIn: parent
                    spacing: root.theme.spacingSmall

                    MaterialIcon {
                        anchors.horizontalCenter: parent.horizontalCenter
                        theme: root.theme
                        text: "notifications_off"
                        iconSize: 32
                        color: root.theme.onSurfaceVariantColour
                        opacity: 0.7
                    }

                    StyledText {
                        anchors.horizontalCenter: parent.horizontalCenter
                        theme: root.theme
                        role: "bodyMedium"
                        color: root.theme.onSurfaceVariantColour
                        text: "Aucune notification du shell"
                    }

                    StyledText {
                        anchors.horizontalCenter: parent.horizontalCenter
                        theme: root.theme
                        role: "labelSmall"
                        color: root.theme.onSurfaceVariantColour
                        opacity: 0.7
                        text: "Le dock reste limité aux notifications propres au shell."
                    }
                }
            }
        }
    }
}
