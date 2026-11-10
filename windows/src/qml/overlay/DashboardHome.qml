import QtQuick
import Caelestia.Platform 1.0
import ".."
import "../components"

Item {
    id: root

    required property var theme

    property date now: new Date()
    property bool editingWeather: !Weather.configured
    property string cityInput: Weather.configured ? Weather.city : ""

    Timer {
        interval: 1000
        repeat: true
        running: true
        onTriggered: root.now = new Date()
    }

    function iconUrl(path) {
        return path ? "file:///" + path.replace(/\\/g, "/") : "";
    }

    function uptimeText() {
        const seconds = System.uptimeSeconds;
        const days = Math.floor(seconds / 86400);
        const hours = Math.floor((seconds % 86400) / 3600);
        const minutes = Math.floor((seconds % 3600) / 60);
        return days > 0 ? days + " j " + hours + " h" : hours + " h " + minutes + " min";
    }

    function rate(kbps) {
        return kbps >= 1000 ? (kbps / 1000).toFixed(1) + " Mb/s"
            : Math.round(kbps) + " kb/s";
    }

    component Tile: Rectangle {
        required property var theme

        radius: theme.roundingLargeIncreased
        color: theme.surfaceContainer
        border.width: 1
        border.color: Qt.alpha(theme.outlineVariant, 0.25)
    }

    Column {
        anchors.fill: parent
        spacing: root.theme.spacingMedium

        Row {
            width: parent.width
            height: 168
            spacing: root.theme.spacingMedium

            Tile {
                width: 220
                height: parent.height
                theme: root.theme

                Column {
                    anchors.fill: parent
                    anchors.margins: root.theme.paddingLarge
                    spacing: root.theme.spacingSmall

                    Row {
                        width: parent.width
                        spacing: root.theme.spacingSmall

                        MaterialIcon {
                            anchors.verticalCenter: parent.verticalCenter
                            theme: root.theme
                            text: Weather.available ? Weather.icon : "cloud_off"
                            iconSize: 38
                            color: root.theme.primary
                        }

                        Column {
                            anchors.verticalCenter: parent.verticalCenter

                            StyledText {
                                theme: root.theme
                                role: "bodyLarge"
                                pointSize: 26
                                weight: Font.DemiBold
                                text: Weather.available ? Weather.temperature : "Météo"
                            }

                            StyledText {
                                theme: root.theme
                                role: "labelSmall"
                                color: root.theme.onSurfaceVariantColour
                                text: Weather.available ? Weather.description : "Non configurée"
                            }
                        }
                    }

                    StyledText {
                        width: parent.width
                        theme: root.theme
                        role: "labelSmall"
                        color: root.theme.onSurfaceVariantColour
                        elide: Text.ElideRight
                        text: Weather.available ? Weather.city : "Choisir une ville"
                    }

                    Item { width: 1; height: 2 }

                    Row {
                        width: parent.width
                        spacing: root.theme.spacingSmall
                        visible: Weather.available

                        StyledText {
                            theme: root.theme
                            role: "labelSmall"
                            color: root.theme.onSurfaceVariantColour
                            text: "Humidité " + Weather.humidity
                        }

                        StyledText {
                            theme: root.theme
                            role: "labelSmall"
                            color: root.theme.onSurfaceVariantColour
                            text: "Vent " + Weather.wind
                        }
                    }
                }

                Row {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: root.theme.paddingLarge
                    spacing: root.theme.spacingSmall
                    visible: !Weather.available

                    Rectangle {
                        width: parent.width - 80
                        height: 34
                        radius: root.theme.roundingSmall
                        color: root.theme.surfaceContainerHigh

                        TextInput {
                            anchors.fill: parent
                            anchors.leftMargin: root.theme.spacingSmall
                            anchors.rightMargin: root.theme.spacingSmall
                            verticalAlignment: TextInput.AlignVCenter
                            color: root.theme.onSurfaceColour
                            font.family: root.theme.bodyFont
                            font.pointSize: 12
                            text: root.cityInput
                            selectByMouse: true
                            onTextChanged: root.cityInput = text
                            onAccepted: Weather.setCity(text)
                        }
                    }

                    M3Button {
                        theme: root.theme
                        icon: "check"
                        implicitWidth: 34
                        implicitHeight: 34
                        onClicked: Weather.setCity(root.cityInput)
                    }
                }
            }

            Tile {
                width: parent.width - 220 - 220 - root.theme.spacingMedium * 2
                height: parent.height
                theme: root.theme

                Row {
                    anchors.fill: parent
                    anchors.margins: root.theme.paddingLarge
                    spacing: root.theme.spacingLarge

                    RoundedClip {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 72
                        height: 72
                        radius: 36

                        Rectangle {
                            anchors.fill: parent
                            color: root.theme.surfaceContainerHigh

                            Image {
                                anchors.fill: parent
                                source: root.iconUrl(User.avatarPath)
                                visible: User.avatarPath !== ""
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                            }

                            MaterialIcon {
                                anchors.centerIn: parent
                                visible: User.avatarPath === ""
                                theme: root.theme
                                text: "account_circle"
                                iconSize: 54
                                color: root.theme.onSurfaceVariantColour
                            }
                        }
                    }

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - 72 - root.theme.spacingLarge
                        spacing: 3

                        StyledText {
                            width: parent.width
                            theme: root.theme
                            role: "bodyLarge"
                            pointSize: 22
                            weight: Font.DemiBold
                            elide: Text.ElideRight
                            text: User.displayName
                        }

                        Row {
                            spacing: root.theme.spacingSmall

                            MaterialIcon {
                                anchors.verticalCenter: parent.verticalCenter
                                theme: root.theme
                                text: "desktop_windows"
                                iconSize: 16
                                color: root.theme.primary
                            }

                            StyledText {
                                anchors.verticalCenter: parent.verticalCenter
                                theme: root.theme
                                role: "bodyMedium"
                                color: root.theme.onSurfaceVariantColour
                                text: User.windowsVersion
                            }
                        }

                        Row {
                            spacing: root.theme.spacingSmall

                            MaterialIcon {
                                anchors.verticalCenter: parent.verticalCenter
                                theme: root.theme
                                text: "computer"
                                iconSize: 16
                                color: root.theme.primary
                            }

                            StyledText {
                                anchors.verticalCenter: parent.verticalCenter
                                theme: root.theme
                                role: "bodyMedium"
                                color: root.theme.onSurfaceVariantColour
                                text: User.hostName
                            }
                        }

                        Row {
                            spacing: root.theme.spacingSmall

                            MaterialIcon {
                                anchors.verticalCenter: parent.verticalCenter
                                theme: root.theme
                                text: "schedule"
                                iconSize: 16
                                color: root.theme.primary
                            }

                            StyledText {
                                anchors.verticalCenter: parent.verticalCenter
                                theme: root.theme
                                role: "bodyMedium"
                                color: root.theme.onSurfaceVariantColour
                                text: "Allumé depuis " + root.uptimeText()
                            }
                        }
                    }
                }
            }

            Tile {
                width: 220
                height: parent.height
                theme: root.theme

                Column {
                    anchors.fill: parent
                    anchors.margins: root.theme.paddingLarge
                    spacing: root.theme.spacingSmall

                    Row {
                        width: parent.width

                        StyledText {
                            theme: root.theme
                            role: "labelSmall"
                            weight: Font.DemiBold
                            color: root.theme.onSurfaceVariantColour
                            text: "MÉDIA"
                        }
                    }

                    Row {
                        width: parent.width
                        spacing: root.theme.spacingSmall

                        RoundedClip {
                            width: 58
                            height: 58
                            radius: root.theme.roundingMedium

                            Rectangle {
                                anchors.fill: parent
                                color: root.theme.surfaceContainerHigh

                                Image {
                                    anchors.fill: parent
                                    source: root.iconUrl(Media.artworkPath)
                                    visible: Media.artworkPath !== ""
                                    fillMode: Image.PreserveAspectCrop
                                    asynchronous: true
                                }

                                MaterialIcon {
                                    anchors.centerIn: parent
                                    visible: Media.artworkPath === ""
                                    theme: root.theme
                                    text: "music_note"
                                    iconSize: 28
                                    color: root.theme.onSurfaceVariantColour
                                }
                            }
                        }

                        Column {
                            width: parent.width - 58 - root.theme.spacingSmall
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 1

                            StyledText {
                                width: parent.width
                                theme: root.theme
                                role: "bodyMedium"
                                weight: Font.Medium
                                elide: Text.ElideRight
                                text: Media.available ? Media.title : "Aucune lecture"
                            }

                            StyledText {
                                width: parent.width
                                theme: root.theme
                                role: "labelSmall"
                                color: root.theme.onSurfaceVariantColour
                                elide: Text.ElideRight
                                text: Media.available ? Media.artist : "GSMTC"
                            }
                        }
                    }

                    Row {
                        spacing: root.theme.spacingSmall
                        visible: Media.available

                        M3Button {
                            theme: root.theme
                            icon: Media.playing ? "pause" : "play_arrow"
                            variant: "tonal"
                            implicitWidth: 36
                            implicitHeight: 32
                            enabled: Media.canPlayPause
                            onClicked: Media.playPause()
                        }

                        M3Button {
                            theme: root.theme
                            icon: "skip_next"
                            variant: "text"
                            implicitWidth: 32
                            implicitHeight: 32
                            enabled: Media.canNext
                            onClicked: Media.next()
                        }
                    }
                }
            }
        }

        Row {
            width: parent.width
            height: parent.height - 168 - root.theme.spacingMedium
            spacing: root.theme.spacingMedium

            Tile {
                width: parent.width * 0.56
                height: parent.height
                theme: root.theme

                DashboardCalendar {
                    anchors.fill: parent
                    anchors.margins: root.theme.paddingLarge
                    theme: root.theme
                    today: root.now
                }
            }

            Tile {
                width: parent.width - parent.width * 0.56 - root.theme.spacingMedium
                height: parent.height
                theme: root.theme

                Column {
                    anchors.fill: parent
                    anchors.margins: root.theme.paddingLarge
                    spacing: root.theme.spacingSmall

                    StyledText {
                        theme: root.theme
                        role: "labelSmall"
                        weight: Font.DemiBold
                        color: root.theme.onSurfaceVariantColour
                        text: "SYSTÈME"
                    }

                    Row {
                        width: parent.width
                        spacing: root.theme.spacingMedium

                        Column {
                            width: (parent.width - root.theme.spacingMedium) / 2
                            spacing: 4

                            StyledText {
                                theme: root.theme
                                role: "bodyMedium"
                                weight: Font.Medium
                                text: "CPU " + Math.round(System.cpuUsage) + " %"
                            }

                            Meter {
                                width: parent.width
                                theme: root.theme
                                value: System.cpuUsage
                            }

                            StyledText {
                                theme: root.theme
                                role: "bodyMedium"
                                weight: Font.Medium
                                text: "RAM " + System.memoryUsedGb.toFixed(1) + " Go"
                            }

                            Meter {
                                width: parent.width
                                theme: root.theme
                                value: System.memoryPercent
                                colour: root.theme.secondary
                            }
                        }

                        Column {
                            width: (parent.width - root.theme.spacingMedium) / 2
                            spacing: 4

                            StyledText {
                                theme: root.theme
                                role: "bodyMedium"
                                weight: Font.Medium
                                text: System.gpuUsageAvailable
                                    ? "GPU " + Math.round(System.gpuUsage) + " %" : "GPU N/D"
                            }

                            Meter {
                                width: parent.width
                                theme: root.theme
                                value: System.gpuUsageAvailable ? System.gpuUsage : 0
                                colour: root.theme.tertiary
                            }

                            StyledText {
                                theme: root.theme
                                role: "bodyMedium"
                                weight: Font.Medium
                                text: "Disque " + Math.round(System.diskPercent) + " %"
                            }

                            Meter {
                                width: parent.width
                                theme: root.theme
                                value: System.diskPercent
                                colour: root.theme.primary
                            }
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: root.theme.outlineVariant
                        opacity: 0.5
                    }

                    Row {
                        width: parent.width
                        spacing: root.theme.spacingLarge

                        Column {
                            StyledText {
                                theme: root.theme
                                role: "labelSmall"
                                color: root.theme.onSurfaceVariantColour
                                text: "Réseau"
                            }
                            StyledText {
                                theme: root.theme
                                role: "bodyMedium"
                                weight: Font.Medium
                                text: Network.connected ? (Network.wifiConnected
                                    ? Network.wifiSsid : "Ethernet") : "Hors ligne"
                            }
                        }

                        Column {
                            StyledText {
                                theme: root.theme
                                role: "labelSmall"
                                color: root.theme.onSurfaceVariantColour
                                text: "Débit"
                            }
                            StyledText {
                                theme: root.theme
                                role: "bodyMedium"
                                weight: Font.Medium
                                text: "↓ " + root.rate(System.networkDownKbps)
                            }
                        }

                        Column {
                            StyledText {
                                theme: root.theme
                                role: "labelSmall"
                                color: root.theme.onSurfaceVariantColour
                                text: "Audio"
                            }
                            StyledText {
                                theme: root.theme
                                role: "bodyMedium"
                                weight: Font.Medium
                                text: Audio.muted ? "Muet" : Audio.volume + " %"
                            }
                        }
                    }
                }
            }
        }
    }
}
