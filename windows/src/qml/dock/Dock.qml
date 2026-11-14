import QtQuick
import QtQuick.Effects
import Caelestia.Platform 1.0
import ".."
import "../components"
import "../components/WindowIcons.js" as WindowIcons

// Application dock. One entry per window of the current desktop, drawn as a
// floating pill inside the strip the AppBar reserved at the bottom of the
// screen — so maximized windows stop above it instead of sliding under.
//
// Styling follows the shell rather than the Windows taskbar: the surface
// colour with the opacity configured in Nexus, full rounding, the same soft
// shadow as the blob, and a halo on hover.
Item {
    id: root

    required property var theme

    readonly property var windows: AppWindows.windows
    readonly property int pillHeight: 64
    readonly property int entityWidth: 52

    Rectangle {
        id: pill

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        // Floating, not glued: the shell's own 10 px frame sits at the very
        // bottom, and a pill touching it read as squashed.
        anchors.bottomMargin: 22
        width: Math.max(row.implicitWidth, emptyHint.implicitWidth + hintSpacing)
            + root.theme.paddingLarge * 2
        height: root.pillHeight
        radius: height / 2
        color: root.theme.surface
        border.width: 1
        border.color: Qt.alpha(root.theme.outlineVariant, 0.35)

        Behavior on width {
            Anim { type: Anim.DefaultSpatial }
        }

        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            blurMax: 20
            shadowBlur: 0.55
            shadowVerticalOffset: 2
            shadowColor: Qt.alpha(root.theme.shadow, root.theme.shadowOpacity * 0.8)
        }

        readonly property int hintSpacing: root.theme.paddingLarge

        Row {
            id: row

            anchors.centerIn: parent
            spacing: root.theme.spacingExtraSmall
            visible: root.windows.length > 0

            Repeater {
                model: root.windows

                delegate: Item {
                    id: entry

                    required property var modelData

                    readonly property bool active: entry.modelData.active
                    readonly property string icon: Platform.windowIcon(entry.modelData.id)

                    width: root.entityWidth
                    height: root.entityWidth

                    Rectangle {
                        anchors.fill: parent
                        radius: 16
                        color: root.theme.layer(root.theme.onSurfaceColour, 0.12)
                        opacity: hover.containsMouse ? 1 : 0

                        Behavior on opacity {
                            Anim { type: Anim.DefaultEffects }
                        }
                    }

                    Image {
                        anchors.centerIn: parent
                        width: 34
                        height: 34
                        source: entry.icon
                        sourceSize.width: 68
                        sourceSize.height: 68
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        visible: status === Image.Ready
                    }

                    // Processes without an extractable icon fall back to the
                    // same category glyph the workspace pills use.
                    MaterialIcon {
                        anchors.centerIn: parent
                        visible: entry.icon === ""
                        theme: root.theme
                        text: WindowIcons.iconFor(entry.modelData.app)
                        iconSize: 26
                        color: entry.active
                            ? root.theme.onSurfaceColour : root.theme.onSurfaceVariantColour
                    }

                    // Running indicator: a wide bar under the focused window,
                    // a small dot under the others.
                    Rectangle {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 2
                        width: entry.active ? 16 : 5
                        height: 3
                        radius: 2
                        color: entry.active ? root.theme.onSurfaceColour : root.theme.outline

                        Behavior on width {
                            Anim { type: Anim.FastSpatial }
                        }
                    }

                    MouseArea {
                        id: hover

                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: AppWindows.focusWindow(entry.modelData.id)
                    }
                }
            }
        }

        Row {
            id: emptyHint

            anchors.centerIn: parent
            spacing: root.theme.spacingSmall
            visible: root.windows.length === 0

            MaterialIcon {
                anchors.verticalCenter: parent.verticalCenter
                theme: root.theme
                text: "apps"
                iconSize: 22
                color: root.theme.onSurfaceVariantColour
                opacity: 0.6
            }

            StyledText {
                anchors.verticalCenter: parent.verticalCenter
                theme: root.theme
                role: "bodySmall"
                color: root.theme.onSurfaceVariantColour
                opacity: 0.7
                text: "Aucune application"
            }
        }
    }
}
