pragma ComponentBehavior: Bound

import QtQuick
import ".."
import "../components"

// Visual shell for the 854 px dashboard. Only navigation and empty states
// are present; no weather/media/performance service is connected.
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

    property bool hoverActive: hoverHalo.containsMouse

    property int activeTab: 0
    readonly property var tabs: [
        { icon: "dashboard", label: "Dashboard" },
        { icon: "queue_music", label: "Media" },
        { icon: "speed", label: "Performance" },
        { icon: "cloud", label: "Weather" }
    ]

    MouseArea {
        anchors.fill: parent
        onClicked: {}
    }

    // Caelestia's top panel hover includes the border rounding around the
    // panel. The halo also covers the 10 px top strip once the panel is open.
    MouseArea {
        id: hoverHalo

        anchors.fill: parent
        anchors.margins: -root.theme.borderRounding
        acceptedButtons: Qt.NoButton
        hoverEnabled: true
    }

    Column {
        anchors.fill: parent
        anchors.margins: root.theme.paddingLarge
        spacing: root.theme.spacingMedium

        Item {
            id: header

            width: parent.width
            height: 50

            Row {
                id: tabRow

                anchors.fill: parent

                Repeater {
                    model: root.tabs

                    Item {
                        id: tabItem

                        required property int index
                        required property var modelData

                        width: tabRow.width / root.tabs.length
                        height: tabRow.height

                        Row {
                            anchors.centerIn: parent
                            spacing: root.theme.spacingSmall

                            MaterialIcon {
                                anchors.verticalCenter: parent.verticalCenter
                                theme: root.theme
                                text: tabItem.modelData.icon
                                iconSize: 18
                                color: root.activeTab === tabItem.index
                                    ? root.theme.primary : root.theme.onSurfaceVariantColour
                            }

                            StyledText {
                                anchors.verticalCenter: parent.verticalCenter
                                theme: root.theme
                                role: "bodySmall"
                                weight: root.activeTab === tabItem.index ? Font.DemiBold : Font.Medium
                                color: root.activeTab === tabItem.index
                                    ? root.theme.primary : root.theme.onSurfaceVariantColour
                                text: tabItem.modelData.label
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.activeTab = tabItem.index
                        }
                    }
                }
            }

            Rectangle {
                id: indicator

                readonly property real tabWidth: header.width / root.tabs.length

                x: root.activeTab * tabWidth + tabWidth / 2 - width / 2
                y: header.height - 4
                width: tabWidth - root.theme.spacingExtraLarge
                height: 3
                radius: height / 2
                color: root.theme.primary

                Behavior on x {
                    Anim {
                        type: Anim.DefaultSpatial
                    }
                }

                Behavior on width {
                    Anim {
                        type: Anim.DefaultSpatial
                    }
                }

                Behavior on color {
                    CAnim {}
                }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: root.theme.outlineVariant
                opacity: 0.6
            }
        }

        Rectangle {
            id: viewport

            width: parent.width
            height: parent.height - header.height - root.theme.spacingMedium
            radius: root.theme.roundingLarge
            color: root.theme.surfaceContainerLow

            Column {
                anchors.centerIn: parent
                spacing: root.theme.spacingSmall

                MaterialIcon {
                    anchors.horizontalCenter: parent.horizontalCenter
                    theme: root.theme
                    text: root.tabs[root.activeTab].icon
                    iconSize: 36
                    color: root.theme.onSurfaceVariantColour
                    opacity: 0.7
                }

                StyledText {
                    anchors.horizontalCenter: parent.horizontalCenter
                    theme: root.theme
                    role: "bodyLarge"
                    weight: Font.DemiBold
                    color: root.theme.onSurfaceVariantColour
                    text: root.tabs[root.activeTab].label
                }

                StyledText {
                    anchors.horizontalCenter: parent.horizontalCenter
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.onSurfaceVariantColour
                    opacity: 0.7
                    text: "Contenu à venir — aucune donnée fictive dans cette fondation."
                }

            }
        }
    }
}
