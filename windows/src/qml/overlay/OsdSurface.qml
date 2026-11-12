import QtQuick
import QtQuick.Layouts
import Caelestia.Platform 1.0
import ".."
import "../components"

// Transient volume / brightness OSD, mirroring the Caelestia top-right pill.
// It lives inside the click-through overlay, so it never takes input.
Item {
    id: root

    required property var theme

    property bool allowed: true

    implicitWidth: 300
    implicitHeight: Osd.value >= 0 ? 68 : 56
    opacity: Osd.visible ? 1 : 0
    visible: allowed && opacity > 0
    scale: Osd.visible ? 1 : 0.96
    transformOrigin: Item.TopRight

    Behavior on opacity {
        Anim {
            type: Anim.DefaultEffects
        }
    }

    Behavior on scale {
        Anim {
            type: Anim.FastSpatial
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: root.theme.roundingExtraLarge
        color: root.theme.surfaceContainerHigh
        border.width: 1
        border.color: root.theme.outlineVariant
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: root.theme.paddingLarge
        spacing: root.theme.spacingMedium

        MaterialIcon {
            Layout.alignment: Qt.AlignVCenter
            theme: root.theme
            text: Osd.icon
            iconSize: 26
            color: root.theme.primary
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: root.theme.spacingExtraSmall

            StyledText {
                theme: root.theme
                role: "labelSmall"
                color: root.theme.onSurfaceVariantColour
                text: Osd.label
            }

            Rectangle {
                Layout.fillWidth: true
                height: 6
                radius: height / 2
                color: root.theme.secondaryContainer
                visible: Osd.value >= 0

                Rectangle {
                    width: Math.max(0, Math.min(1, Osd.value / 100)) * parent.width
                    height: parent.height
                    radius: parent.radius
                    color: root.theme.primary

                    Behavior on width {
                        Anim {
                            type: Anim.StandardSmall
                        }
                    }
                }
            }
        }

        StyledText {
            Layout.alignment: Qt.AlignVCenter
            theme: root.theme
            role: "bodyMedium"
            weight: Font.DemiBold
            visible: Osd.value >= 0
            text: Osd.value >= 0 ? Osd.value + " %" : ""
        }
    }
}
