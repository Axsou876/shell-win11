import QtQuick
import Caelestia.Platform 1.0
import "../components"

// Control Centre dock: real Windows services, favourites, quick actions and
// confirmed session actions. The blob surface is drawn by ScreenSurface.
Item {
    id: root

    required property var theme
    required property var controller
    required property string screenName

    property real deformScaleX: 1
    property real deformScaleY: 1

    transform: Scale {
        xScale: root.deformScaleX
        yScale: root.deformScaleY
        origin.x: root.width / 2
        origin.y: root.height / 2
    }

    property date now: new Date()

    Timer {
        interval: 1000
        repeat: true
        running: true
        onTriggered: root.now = new Date()
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
        border.width: 1
        border.color: Qt.alpha(root.theme.outlineVariant, 0.25)

        Column {
            anchors.fill: parent
            anchors.margins: root.theme.paddingLarge
            spacing: root.theme.spacingLarge

            Row {
                width: parent.width

                Column {
                    spacing: 1

                    StyledText {
                        theme: root.theme
                        role: "bodyLarge"
                        pointSize: 26
                        weight: Font.DemiBold
                        text: Qt.formatDateTime(root.now, "HH:mm")
                    }

                    StyledText {
                        theme: root.theme
                        role: "labelSmall"
                        color: root.theme.onSurfaceVariantColour
                        text: Qt.formatDateTime(root.now, "dddd d MMMM")
                    }
                }

                Item { width: parent.width - 90; height: 1 }

                M3Button {
                    anchors.verticalCenter: parent.verticalCenter
                    theme: root.theme
                    icon: "close"
                    variant: "text"
                    implicitWidth: 40
                    implicitHeight: 40
                    onClicked: root.controller.closePanel()
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: root.theme.outlineVariant
                opacity: 0.5
            }

            StyledText {
                theme: root.theme
                role: "labelSmall"
                weight: Font.DemiBold
                color: root.theme.onSurfaceVariantColour
                text: "ACCÈS RAPIDE"
            }

            Grid {
                width: parent.width
                columns: 2
                columnSpacing: root.theme.spacingSmall
                rowSpacing: root.theme.spacingSmall

                readonly property real cellWidth: (width - columnSpacing) / 2

                M3Button {
                    width: parent.cellWidth
                    theme: root.theme
                    text: "Applications"
                    icon: "apps"
                    variant: "tonal"
                    onClicked: root.controller.togglePanel("launcher", root.screenName, 0)
                }

                M3Button {
                    width: parent.cellWidth
                    theme: root.theme
                    text: "Dashboard"
                    icon: "dashboard"
                    variant: "tonal"
                    onClicked: root.controller.togglePanel("dashboard", root.screenName, 0)
                }

                M3Button {
                    width: parent.cellWidth
                    theme: root.theme
                    text: "Explorateur"
                    icon: "folder"
                    variant: "text"
                    onClicked: Qt.openUrlExternally("file:///C:/")
                }

                M3Button {
                    width: parent.cellWidth
                    theme: root.theme
                    text: "Paramètres"
                    icon: "settings"
                    variant: "text"
                    onClicked: Qt.openUrlExternally("ms-settings:")
                }
            }

            StyledText {
                theme: root.theme
                role: "labelSmall"
                weight: Font.DemiBold
                color: root.theme.onSurfaceVariantColour
                text: "FAVORIS"
            }

            Column {
                width: parent.width
                spacing: 2

                Repeater {
                    model: Apps.favoriteApps

                    M3ListItem {
                        required property var modelData

                        width: parent.width
                        theme: root.theme
                        icon: "apps"
                        title: modelData.name
                        onClicked: Apps.launchById(modelData.id)
                    }
                }

                StyledText {
                    width: parent.width
                    visible: Apps.favoriteApps.length === 0
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.onSurfaceVariantColour
                    wrapMode: Text.WordWrap
                    text: "Ajoutez des favoris avec l'étoile dans le launcher."
                }
            }

            StyledText {
                theme: root.theme
                role: "labelSmall"
                weight: Font.DemiBold
                color: root.theme.onSurfaceVariantColour
                text: "AUDIO"
            }

            Row {
                width: parent.width
                spacing: root.theme.spacingSmall

                MaterialIcon {
                    anchors.verticalCenter: parent.verticalCenter
                    theme: root.theme
                    text: Audio.muted ? "volume_off" : "volume_up"
                    iconSize: 22
                    color: root.theme.primary
                }

                StyledText {
                    anchors.verticalCenter: parent.verticalCenter
                    theme: root.theme
                    role: "bodyMedium"
                    width: 45
                    text: Audio.volume + " %"
                }

                M3Slider {
                    id: sideVolume

                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - 120
                    theme: root.theme
                    value: Audio.volume / 100
                    onMoved: Audio.setVolumePercent(Math.round(value * 100))
                }

                M3Button {
                    anchors.verticalCenter: parent.verticalCenter
                    theme: root.theme
                    icon: Audio.muted ? "volume_off" : "volume_up"
                    variant: "text"
                    implicitWidth: 34
                    implicitHeight: 32
                    onClicked: Audio.toggleMute()
                }
            }

            Binding {
                target: sideVolume
                property: "value"
                value: Audio.volume / 100
                when: !sideVolume.dragging
            }

            StyledText {
                theme: root.theme
                role: "labelSmall"
                weight: Font.DemiBold
                color: root.theme.onSurfaceVariantColour
                text: "ÉTAT DU SYSTÈME"
            }

            Row {
                width: parent.width
                spacing: root.theme.spacingSmall

                Rectangle {
                    width: (parent.width - root.theme.spacingSmall * 2) / 3
                    height: 72
                    radius: root.theme.roundingMedium
                    color: root.theme.surfaceContainer

                    Column {
                        anchors.centerIn: parent
                        spacing: 2

                        MaterialIcon {
                            anchors.horizontalCenter: parent.horizontalCenter
                            theme: root.theme
                            text: Network.connected ? "wifi" : "wifi_off"
                            iconSize: 22
                            color: root.theme.primary
                        }
                        StyledText {
                            anchors.horizontalCenter: parent.horizontalCenter
                            theme: root.theme
                            role: "labelSmall"
                            text: Network.connected ? "Connecté" : "Hors ligne"
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.controller.togglePanel("network", root.screenName, 0)
                    }
                }

                Rectangle {
                    width: (parent.width - root.theme.spacingSmall * 2) / 3
                    height: 72
                    radius: root.theme.roundingMedium
                    color: root.theme.surfaceContainer

                    Column {
                        anchors.centerIn: parent
                        spacing: 2

                        MaterialIcon {
                            anchors.horizontalCenter: parent.horizontalCenter
                            theme: root.theme
                            text: Bluetooth.enabled ? "bluetooth" : "bluetooth_disabled"
                            iconSize: 22
                            color: root.theme.primary
                        }
                        StyledText {
                            anchors.horizontalCenter: parent.horizontalCenter
                            theme: root.theme
                            role: "labelSmall"
                            text: Bluetooth.present
                                ? (Bluetooth.enabled ? "Activé" : "Désactivé") : "Absent"
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.controller.togglePanel("bluetooth", root.screenName, 0)
                    }
                }

                Rectangle {
                    width: (parent.width - root.theme.spacingSmall * 2) / 3
                    height: 72
                    radius: root.theme.roundingMedium
                    color: root.theme.surfaceContainer

                    Column {
                        anchors.centerIn: parent
                        spacing: 2

                        MaterialIcon {
                            anchors.horizontalCenter: parent.horizontalCenter
                            theme: root.theme
                            text: Battery.charging ? "battery_charging_full" : "battery_full"
                            iconSize: 22
                            color: root.theme.primary
                        }
                        StyledText {
                            anchors.horizontalCenter: parent.horizontalCenter
                            theme: root.theme
                            role: "labelSmall"
                            text: Battery.known ? Battery.level + " %" : "N/D"
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.controller.togglePanel("battery", root.screenName, 0)
                    }
                }
            }

            Item { width: 1; height: 1 }

            M3Button {
                width: parent.width
                theme: root.theme
                text: "Alimentation"
                icon: "power_settings_new"
                variant: "tonal"
                onClicked: root.controller.togglePanel("power", root.screenName, 0)
            }
        }
    }
}
