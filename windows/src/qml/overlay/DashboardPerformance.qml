import QtQuick
import Caelestia.Platform 1.0
import ".."
import "../components"

// Performance tab reconstructed from Performance.qml and performance/*:
// HeroCard CPU/GPU, StorageCard, NetworkCard, MemoryCard. Values are live
// Windows measurements; CPU/GPU temperature stays explicit N/D.
Item {
    id: root

    required property var theme

    function formatRate(kbps) {
        if (kbps >= 1000)
            return (kbps / 1000).toFixed(1) + " Mb/s";
        return Math.round(kbps) + " kb/s";
    }

    component Card: Rectangle {
        id: card

        required property var theme

        color: theme.surfaceContainer
        border.width: 1
        border.color: Qt.alpha(theme.outlineVariant, 0.25)
    }

    component HeroCard: Card {
        id: hero

        required property string icon
        required property string label
        required property string subLabel
        required property color accent
        required property real usage
        required property bool usageAvailable

        radius: root.theme.roundingExtraLarge

        CircularProgress {
            x: 16
            y: 16
            width: 100
            height: 100
            theme: root.theme
            value: hero.usageAvailable ? hero.usage / 100 : 0
            strokeWidth: 4
            fgColour: hero.accent

            MaterialIcon {
                anchors.centerIn: parent
                theme: root.theme
                text: hero.icon
                iconSize: 24
                color: hero.accent
            }
        }

        Column {
            x: 132
            y: 18
            width: hero.width - 148
            spacing: 2

            StyledText {
                theme: root.theme
                role: "bodyLarge"
                pointSize: 16
                weight: Font.DemiBold
                color: hero.accent
                text: hero.label
            }

            StyledText {
                width: parent.width
                theme: root.theme
                role: "bodySmall"
                color: root.theme.onSurfaceVariantColour
                elide: Text.ElideRight
                text: hero.subLabel
            }
        }

        Row {
            x: 16
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 18
            spacing: 6

            MaterialIcon {
                anchors.verticalCenter: parent.verticalCenter
                theme: root.theme
                text: "thermometer"
                iconSize: 18
                color: hero.accent
            }

            StyledText {
                anchors.verticalCenter: parent.verticalCenter
                theme: root.theme
                role: "bodySmall"
                color: root.theme.onSurfaceVariantColour
                text: "Température N/D"
            }
        }

        StyledText {
            anchors.right: usageShape.right
            anchors.bottom: usageShape.top
            anchors.bottomMargin: 2
            theme: root.theme
            role: "bodySmall"
            color: root.theme.onSurfaceVariantColour
            text: "Usage"
        }

        Rectangle {
            id: usageShape

            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 12
            width: 100
            height: 100
            radius: 28
            color: root.theme.secondaryContainer

            StyledText {
                anchors.centerIn: parent
                theme: root.theme
                role: "bodyLarge"
                pointSize: 24
                weight: Font.DemiBold
                color: hero.accent
                text: hero.usageAvailable ? Math.round(hero.usage) + "%" : "N/D"
            }
        }
    }

    HeroCard {
        x: 0
        y: 0
        width: System.gpuUsageAvailable ? (root.width - root.theme.spacingMedium) / 2 : root.width
        height: 180
        theme: root.theme
        icon: "memory"
        label: "CPU"
        subLabel: System.cpuName
        accent: root.theme.primary
        usage: System.cpuUsage
        usageAvailable: true
    }

    HeroCard {
        visible: System.gpuUsageAvailable
        x: (root.width - root.theme.spacingMedium) / 2 + root.theme.spacingMedium
        y: 0
        width: (root.width - root.theme.spacingMedium) / 2
        height: 180
        theme: root.theme
        icon: "desktop_windows"
        label: "GPU"
        subLabel: System.gpuName !== "" ? System.gpuName : "Aucun GPU détecté"
        accent: root.theme.secondary
        usage: System.gpuUsage
        usageAvailable: System.gpuUsageAvailable
    }

    // --- Storage -----------------------------------------------------------
    Card {
        x: 0
        y: 192
        width: 280
        height: root.height - 192
        radius: root.theme.roundingExtraLargeIncreased
        theme: root.theme

        CircularProgress {
            id: storageRing

            x: 24
            anchors.verticalCenter: parent.verticalCenter
            width: 132
            height: 132
            theme: root.theme
            value: System.diskPercent / 100
            startAngle: -225
            sweepAngle: 270
            strokeWidth: 6
            fgColour: root.theme.secondary

            Column {
                anchors.centerIn: parent
                spacing: 0

                MaterialIcon {
                    anchors.horizontalCenter: parent.horizontalCenter
                    theme: root.theme
                    text: "hard_drive"
                    iconSize: 20
                    color: root.theme.secondary
                }

                StyledText {
                    anchors.horizontalCenter: parent.horizontalCenter
                    theme: root.theme
                    role: "bodyLarge"
                    pointSize: 22
                    weight: Font.DemiBold
                    color: root.theme.secondary
                    text: Math.round(System.diskPercent) + "%"
                }

                StyledText {
                    anchors.horizontalCenter: parent.horizontalCenter
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.onSurfaceVariantColour
                    text: "Utilisé"
                }
            }
        }

        Column {
            x: storageRing.x + storageRing.width + 18
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width - x - 16
            spacing: 4

            StyledText {
                theme: root.theme
                role: "titleSmall"
                weight: Font.DemiBold
                text: "Stockage"
            }

            StyledText {
                theme: root.theme
                role: "bodyMedium"
                color: root.theme.secondary
                text: System.diskUsedGb.toFixed(0) + " / " + System.diskTotalGb.toFixed(0) + " Go"
            }
        }
    }

    // --- Network -----------------------------------------------------------
    Card {
        id: networkCard

        x: 292
        y: 192
        width: 380
        height: root.height - 192
        radius: root.theme.roundingExtraLarge
        theme: root.theme

        property var downValues: []
        property var upValues: []

        Timer {
            interval: 1000
            running: true
            repeat: true
            triggeredOnStart: true
            onTriggered: {
                let down = networkCard.downValues.slice();
                let up = networkCard.upValues.slice();
                down.push(Math.max(0, System.networkDownKbps));
                up.push(Math.max(0, System.networkUpKbps));
                if (down.length > 60)
                    down.shift();
                if (up.length > 60)
                    up.shift();
                networkCard.downValues = down;
                networkCard.upValues = up;
            }
        }

        readonly property real graphMax: {
            let maxValue = 1024;
            for (let i = 0; i < downValues.length; ++i)
                maxValue = Math.max(maxValue, downValues[i]);
            for (let i = 0; i < upValues.length; ++i)
                maxValue = Math.max(maxValue, upValues[i]);
            return maxValue * 1.15;
        }

        Row {
            id: networkHeader

            x: 16
            y: 14
            spacing: 6

            MaterialIcon {
                anchors.verticalCenter: parent.verticalCenter
                theme: root.theme
                text: "swap_vert"
                iconSize: 20
                color: root.theme.primary
            }

            StyledText {
                anchors.verticalCenter: parent.verticalCenter
                theme: root.theme
                role: "titleSmall"
                weight: Font.DemiBold
                text: "Réseau"
            }
        }

        Item {
            id: graph

            x: 16
            y: networkHeader.y + networkHeader.height + 10
            width: parent.width - 32
            height: parent.height - y - 104

            Sparkline {
                anchors.fill: parent
                values: networkCard.downValues
                maximum: networkCard.graphMax
                lineColor: root.theme.tertiary
            }

            Sparkline {
                anchors.fill: parent
                values: networkCard.upValues
                maximum: networkCard.graphMax
                lineColor: root.theme.secondary
            }

            StyledText {
                anchors.centerIn: parent
                visible: networkCard.downValues.length < 2
                theme: root.theme
                role: "bodySmall"
                color: root.theme.outline
                text: "Collecte des données…"
            }
        }

        Column {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 16
            spacing: 6

            Row {
                width: parent.width
                spacing: 6

                MaterialIcon {
                    anchors.verticalCenter: parent.verticalCenter
                    theme: root.theme
                    text: "download"
                    iconSize: 18
                    color: root.theme.tertiary
                }

                StyledText {
                    anchors.verticalCenter: parent.verticalCenter
                    theme: root.theme
                    role: "bodySmall"
                    color: root.theme.onSurfaceVariantColour
                    text: "Téléchargement"
                }

                Item { width: parent.width - 220; height: 1 }

                StyledText {
                    anchors.verticalCenter: parent.verticalCenter
                    theme: root.theme
                    role: "bodySmall"
                    weight: Font.Medium
                    color: root.theme.tertiary
                    text: root.formatRate(System.networkDownKbps)
                }
            }

            Row {
                width: parent.width
                spacing: 6

                MaterialIcon {
                    anchors.verticalCenter: parent.verticalCenter
                    theme: root.theme
                    text: "upload"
                    iconSize: 18
                    color: root.theme.secondary
                }

                StyledText {
                    anchors.verticalCenter: parent.verticalCenter
                    theme: root.theme
                    role: "bodySmall"
                    color: root.theme.onSurfaceVariantColour
                    text: "Envoi"
                }

                Item { width: parent.width - 220; height: 1 }

                StyledText {
                    anchors.verticalCenter: parent.verticalCenter
                    theme: root.theme
                    role: "bodySmall"
                    weight: Font.Medium
                    color: root.theme.secondary
                    text: root.formatRate(System.networkUpKbps)
                }
            }
        }
    }

    // --- Memory ------------------------------------------------------------
    Card {
        x: 684
        y: 192
        width: root.width - 684
        height: root.height - 192
        radius: root.theme.roundingMedium
        theme: root.theme

        CircularProgress {
            id: memoryRing

            anchors.horizontalCenter: parent.horizontalCenter
            y: 22
            width: 150
            height: 150
            theme: root.theme
            value: System.memoryPercent / 100
            startAngle: -225
            sweepAngle: 270
            strokeWidth: 7
            fgColour: root.theme.tertiary

            Column {
                anchors.centerIn: parent
                spacing: 0

                StyledText {
                    anchors.horizontalCenter: parent.horizontalCenter
                    theme: root.theme
                    role: "bodyLarge"
                    pointSize: 26
                    weight: Font.DemiBold
                    color: root.theme.tertiary
                    text: Math.round(System.memoryPercent) + "%"
                }

                StyledText {
                    anchors.horizontalCenter: parent.horizontalCenter
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.onSurfaceVariantColour
                    text: "Utilisé"
                }
            }
        }

        StyledText {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: memoryRing.bottom
            anchors.topMargin: 12
            theme: root.theme
            role: "titleSmall"
            weight: Font.DemiBold
            text: "Mémoire"
        }

        StyledText {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: memoryRing.bottom
            anchors.topMargin: 34
            theme: root.theme
            role: "bodyMedium"
            text: System.memoryUsedGb.toFixed(1) + " / " + System.memoryTotalGb.toFixed(1) + " Go"
        }
    }

    // Linux hides the battery tank on non-laptop devices; Windows provides no
    // laptop battery on this machine, so no empty decorative tank is drawn.
}
