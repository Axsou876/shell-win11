import QtQuick
import Caelestia.Platform 1.0
import ".."
import "../components"

Item {
    id: root

    required property var theme

    Column {
        anchors.fill: parent
        anchors.margins: root.theme.paddingLarge
        spacing: root.theme.spacingMedium

        Row {
            width: parent.width
            height: 30

            StyledText {
                anchors.verticalCenter: parent.verticalCenter
                theme: root.theme
                role: "bodyLarge"
                weight: Font.DemiBold
                text: "Bureau actuel"
            }

            StyledText {
                anchors.verticalCenter: parent.verticalCenter
                theme: root.theme
                role: "labelSmall"
                color: root.theme.onSurfaceVariantColour
                text: " · " + AppWindows.count + " fenêtre"
                    + (AppWindows.count > 1 ? "s" : "")
            }

            Item { width: parent.width - 260; height: 1 }

            M3Button {
                anchors.verticalCenter: parent.verticalCenter
                theme: root.theme
                icon: "refresh"
                variant: "text"
                implicitWidth: 36
                implicitHeight: 32
                onClicked: AppWindows.refresh()
            }
        }

        Grid {
            id: grid

            width: parent.width
            height: parent.height - 42
            columns: 3
            columnSpacing: root.theme.spacingMedium
            rowSpacing: root.theme.spacingMedium

            readonly property real cellWidth: (width - columnSpacing * 2) / 3

            Repeater {
                model: AppWindows.windows

                Rectangle {
                    required property var modelData

                    width: grid.cellWidth
                    height: 94
                    radius: root.theme.roundingLarge
                    color: root.theme.surfaceContainer
                    border.width: modelData.active ? 2 : 1
                    border.color: modelData.active
                        ? root.theme.primary
                        : Qt.alpha(root.theme.outlineVariant, 0.25)

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: AppWindows.focusWindow(modelData.id)
                    }

                    Row {
                        anchors.fill: parent
                        anchors.margins: root.theme.paddingMedium
                        spacing: root.theme.spacingMedium

                        Item {
                            width: 42
                            height: 42
                            anchors.verticalCenter: parent.verticalCenter

                            Rectangle {
                                anchors.fill: parent
                                radius: root.theme.roundingMedium
                                color: root.theme.secondaryContainer

                                MaterialIcon {
                                    anchors.centerIn: parent
                                    theme: root.theme
                                    text: modelData.active ? "web_asset" : "window"
                                    iconSize: 22
                                    color: root.theme.onSecondaryContainerColour
                                }
                            }
                        }

                        Column {
                            width: parent.width - 42 - root.theme.spacingMedium
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 2

                            StyledText {
                                width: parent.width
                                theme: root.theme
                                role: "labelSmall"
                                color: root.theme.primary
                                elide: Text.ElideRight
                                text: modelData.app
                            }

                            StyledText {
                                width: parent.width
                                theme: root.theme
                                role: "bodyMedium"
                                weight: Font.Medium
                                wrapMode: Text.Wrap
                                maximumLineCount: 2
                                elide: Text.ElideRight
                                text: modelData.title
                            }
                        }
                    }
                }
            }
        }

        StyledText {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: AppWindows.count === 0
            theme: root.theme
            role: "bodyMedium"
            color: root.theme.onSurfaceVariantColour
            text: "Aucune fenêtre sur le bureau actuel."
        }
    }
}
