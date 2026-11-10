import QtQuick
import Caelestia.Platform 1.0
import ".."
import "../components"

Item {
    id: root

    required property var theme

    property date now: new Date()

    Timer {
        interval: 1000
        repeat: true
        running: true
        onTriggered: root.now = new Date()
    }

    function timeText() {
        return Qt.formatDateTime(root.now, "HH:mm");
    }

    function dateText() {
        return Qt.formatDateTime(root.now, "dddd d MMMM");
    }

    function uptimeText() {
        const seconds = System.uptimeSeconds;
        const days = Math.floor(seconds / 86400);
        const hours = Math.floor((seconds % 86400) / 3600);
        const minutes = Math.floor((seconds % 3600) / 60);
        return days > 0 ? days + " j " + hours + " h" : hours + " h " + minutes + " min";
    }

    Column {
        anchors.fill: parent
        spacing: root.theme.spacingMedium

        DashCard {
            width: parent.width
            height: 104
            contentHeight: 42
            theme: root.theme
            title: "Aujourd'hui"
            icon: "schedule"

            Row {
                anchors.fill: parent
                spacing: root.theme.spacingLarge

                StyledText {
                    anchors.verticalCenter: parent.verticalCenter
                    theme: root.theme
                    role: "bodyLarge"
                    pointSize: 38
                    weight: Font.DemiBold
                    color: root.theme.primary
                    text: root.timeText()
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 1

                    StyledText {
                        theme: root.theme
                        role: "bodyMedium"
                        weight: Font.Medium
                        text: root.dateText()
                    }

                    StyledText {
                        theme: root.theme
                        role: "labelSmall"
                        color: root.theme.onSurfaceVariantColour
                        text: "Allumé depuis " + root.uptimeText()
                    }
                }

                Item {
                    width: parent.width - 340
                    height: 1
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 1

                    StyledText {
                        anchors.right: parent.right
                        theme: root.theme
                        role: "labelSmall"
                        color: root.theme.onSurfaceVariantColour
                        text: Audio.present ? Audio.deviceName : "Aucune sortie audio"
                        elide: Text.ElideRight
                        width: 220
                        horizontalAlignment: Text.AlignRight
                    }

                    StyledText {
                        anchors.right: parent.right
                        theme: root.theme
                        role: "labelSmall"
                        color: root.theme.onSurfaceVariantColour
                        opacity: 0.7
                        text: Battery.present
                            ? (Battery.onAc ? "Sur secteur" : "Sur batterie") + " · "
                                + (Battery.level >= 0 ? Battery.level + " %" : "niveau inconnu")
                            : "Pas de batterie"
                    }
                }
            }
        }

        Grid {
            width: parent.width
            columns: 2
            columnSpacing: root.theme.spacingMedium
            rowSpacing: root.theme.spacingMedium

            readonly property real cellWidth: (width - columnSpacing) / 2

            DashCard {
                width: parent.cellWidth
                height: 156
                contentHeight: 84
                theme: root.theme
                title: "Système"
                icon: "memory"

                Column {
                    anchors.fill: parent
                    spacing: root.theme.spacingSmall

                    Row {
                        width: parent.width
                        spacing: root.theme.spacingSmall

                        StyledText {
                            theme: root.theme
                            role: "bodyLarge"
                            weight: Font.DemiBold
                            text: Math.round(System.cpuUsage) + " %"
                        }

                        StyledText {
                            anchors.verticalCenter: parent.verticalCenter
                            theme: root.theme
                            role: "labelSmall"
                            color: root.theme.onSurfaceVariantColour
                            text: "CPU"
                        }

                        Item { width: parent.width - 100; height: 1 }

                        StyledText {
                            anchors.verticalCenter: parent.verticalCenter
                            theme: root.theme
                            role: "labelSmall"
                            color: root.theme.onSurfaceVariantColour
                            text: System.memoryUsedGb.toFixed(1) + " / "
                                + System.memoryTotalGb.toFixed(1) + " Go"
                        }
                    }

                    Meter {
                        width: parent.width
                        theme: root.theme
                        value: System.cpuUsage
                    }

                    Meter {
                        width: parent.width
                        theme: root.theme
                        value: System.memoryPercent
                        colour: root.theme.secondary
                    }

                    StyledText {
                        width: parent.width
                        theme: root.theme
                        role: "labelSmall"
                        color: root.theme.onSurfaceVariantColour
                        opacity: 0.7
                        elide: Text.ElideRight
                        text: System.gpuAvailable
                            ? System.gpuName + " · disque " + Math.round(System.diskPercent) + " %"
                            : "Disque " + Math.round(System.diskPercent) + " %"
                    }
                }
            }

            DashCard {
                width: parent.cellWidth
                height: 156
                contentHeight: 84
                theme: root.theme
                title: "Réseau"
                icon: "wifi"

                Column {
                    anchors.fill: parent
                    spacing: root.theme.spacingSmall

                    Row {
                        width: parent.width
                        spacing: root.theme.spacingSmall

                        MaterialIcon {
                            anchors.verticalCenter: parent.verticalCenter
                            theme: root.theme
                            text: Network.connectionType === "ethernet" ? "cable"
                                : Network.connected ? "wifi" : "wifi_off"
                            iconSize: 22
                            color: Network.connected ? root.theme.primary
                                : root.theme.onSurfaceVariantColour
                        }

                        Column {
                            anchors.verticalCenter: parent.verticalCenter

                            StyledText {
                                theme: root.theme
                                role: "bodyMedium"
                                weight: Font.Medium
                                text: {
                                    if (!Network.known)
                                        return "État inconnu";
                                    if (Network.connectionType === "ethernet")
                                        return Network.ethernetName || "Ethernet";
                                    if (Network.wifiConnected)
                                        return Network.wifiSsid;
                                    return Network.connected ? "Connecté" : "Déconnecté";
                                }
                            }

                            StyledText {
                                theme: root.theme
                                role: "labelSmall"
                                color: root.theme.onSurfaceVariantColour
                                text: Network.connected
                                    ? (Network.connectionType === "wifi"
                                        ? "Wi-Fi · signal " + Network.wifiSignal + " %"
                                        : "Ethernet · connecté")
                                    : "Aucune connexion active"
                            }
                        }
                    }

                    Item { width: 1; height: 2 }

                    Row {
                        width: parent.width
                        spacing: root.theme.spacingLarge

                        Column {
                            StyledText {
                                theme: root.theme
                                role: "labelSmall"
                                color: root.theme.onSurfaceVariantColour
                                text: "Descendant"
                            }
                            StyledText {
                                theme: root.theme
                                role: "bodyMedium"
                                weight: Font.Medium
                                text: root.formatRate(System.networkDownKbps)
                            }
                        }

                        Column {
                            StyledText {
                                theme: root.theme
                                role: "labelSmall"
                                color: root.theme.onSurfaceVariantColour
                                text: "Montant"
                            }
                            StyledText {
                                theme: root.theme
                                role: "bodyMedium"
                                weight: Font.Medium
                                text: root.formatRate(System.networkUpKbps)
                            }
                        }
                    }
                }
            }

            DashCard {
                width: parent.cellWidth
                height: 156
                contentHeight: 84
                theme: root.theme
                title: "Audio et énergie"
                icon: "volume_up"

                Column {
                    anchors.fill: parent
                    spacing: root.theme.spacingSmall

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
                            role: "bodyLarge"
                            weight: Font.DemiBold
                            text: Audio.volume + " %"
                        }

                        Item { width: parent.width - 130; height: 1 }

                        M3Button {
                            anchors.verticalCenter: parent.verticalCenter
                            theme: root.theme
                            icon: Audio.muted ? "volume_off" : "volume_up"
                            variant: "text"
                            implicitWidth: 36
                            implicitHeight: 32
                            onClicked: Audio.toggleMute()
                        }
                    }

                    M3Slider {
                        id: volume

                        width: parent.width
                        theme: root.theme
                        value: Audio.volume / 100
                        onMoved: Audio.setVolumePercent(Math.round(value * 100))
                    }

                    Binding {
                        target: volume
                        property: "value"
                        value: Audio.volume / 100
                        when: !volume.dragging
                    }

                    StyledText {
                        width: parent.width
                        theme: root.theme
                        role: "labelSmall"
                        color: root.theme.onSurfaceVariantColour
                        elide: Text.ElideRight
                        text: Audio.present
                            ? Audio.deviceName
                            : "Aucun périphérique de sortie"
                    }
                }
            }

            DashCard {
                width: parent.cellWidth
                height: 156
                contentHeight: 84
                theme: root.theme
                title: "Média"
                icon: "queue_music"

                Row {
                    anchors.fill: parent
                    spacing: root.theme.spacingMedium

                    RoundedClip {
                        width: 84
                        height: 84
                        radius: root.theme.roundingMedium

                        Rectangle {
                            anchors.fill: parent
                            color: root.theme.surfaceContainerHigh

                            Image {
                                anchors.fill: parent
                                source: Media.artworkPath
                                    ? "file:///" + Media.artworkPath.replace(/\\/g, "/") : ""
                                visible: Media.artworkPath !== ""
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                            }

                            MaterialIcon {
                                anchors.centerIn: parent
                                visible: !Media.artworkPath
                                theme: root.theme
                                text: "music_note"
                                iconSize: 34
                                color: root.theme.onSurfaceVariantColour
                            }
                        }
                    }

                    Column {
                        width: parent.width - 84 - root.theme.spacingMedium
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 2

                        StyledText {
                            width: parent.width
                            theme: root.theme
                            role: "bodyMedium"
                            weight: Font.Medium
                            elide: Text.ElideRight
                            text: Media.available ? Media.title : "Aucune lecture active"
                        }

                        StyledText {
                            width: parent.width
                            theme: root.theme
                            role: "labelSmall"
                            color: root.theme.onSurfaceVariantColour
                            elide: Text.ElideRight
                            text: Media.available ? Media.artist : "Lancez un lecteur compatible GSMTC"
                        }

                        Row {
                            spacing: root.theme.spacingSmall
                            visible: Media.available

                            M3Button {
                                theme: root.theme
                                icon: Media.playing ? "pause" : "play_arrow"
                                variant: "tonal"
                                implicitWidth: 38
                                implicitHeight: 34
                                enabled: Media.canPlayPause
                                onClicked: Media.playPause()
                            }

                            M3Button {
                                theme: root.theme
                                icon: "skip_next"
                                variant: "text"
                                implicitWidth: 34
                                implicitHeight: 34
                                enabled: Media.canNext
                                onClicked: Media.next()
                            }
                        }
                    }
                }
            }
        }
    }

    function formatRate(kbps) {
        if (kbps >= 1000)
            return (kbps / 1000).toFixed(1) + " Mb/s";
        return Math.round(kbps) + " kb/s";
    }
}
