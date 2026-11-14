import QtQuick
import QtQuick.Layouts
import Caelestia.Platform 1.0
import ".."
import "../components"

// Performance tab, rebuilt on the Linux layout (modules/dashboard/Performance.qml
// and dashboard/performance/*): a row of CPU/GPU hero cards above a row of
// storage / network / memory cards, all sharing the width equally through
// RowLayout. The values are live Windows measurements; CPU/GPU temperature is
// the one thing GSMTC has no equivalent for, so it stays explicitly N/D.
Item {
    id: root

    required property var theme

    // False while the dashboard is closed: the page stays loaded, but its
    // gauges must not animate off-screen (see CircularProgress.animate).
    property bool animations: true

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

    // Caelestia's HeroCard: usage ring top-left, name and model beside it, the
    // temperature block bottom-left, and a large shape bottom-right whose form
    // follows the load — Cookie4Sided below 40 %, Sunny below 80 %, SoftBurst
    // above. The shape is the card's identity, not decoration.
    component HeroCard: Card {
        id: hero

        required property string icon
        required property string label
        required property string subLabel
        required property color accent
        required property real usage
        required property bool usageAvailable
        property bool animations: true

        readonly property real ratio: usageAvailable
            ? Math.max(0, Math.min(1, usage / 100)) : 0

        radius: root.theme.roundingExtraLarge

        CircularProgress {
            id: usageRing

            x: root.theme.paddingLarge
            y: root.theme.paddingLarge
            width: hero.theme.iconMedium + root.theme.paddingMedium * 2
            height: width
            theme: root.theme
            animate: hero.animations
            value: hero.ratio
            strokeWidth: 4
            fgColour: hero.accent

            MaterialIcon {
                anchors.centerIn: parent
                theme: root.theme
                text: hero.icon
                iconSize: root.theme.iconMedium
                color: hero.accent
            }
        }

        Column {
            anchors.left: usageRing.right
            anchors.right: usageShape.left
            anchors.leftMargin: root.theme.spacingLarge
            anchors.rightMargin: root.theme.spacingMedium
            anchors.verticalCenter: usageRing.verticalCenter
            spacing: root.theme.spacingExtraSmall

            StyledText {
                theme: root.theme
                role: "titleSmall"
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
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.margins: root.theme.paddingLargeIncreased
            spacing: root.theme.spacingExtraSmall

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
                text: "N/D"
            }
        }

        MaterialShape {
            id: usageShape

            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: root.theme.paddingMedium
            implicitSize: 100
            color: root.theme.secondaryContainer
            shape: hero.ratio >= 0.8 ? MaterialShape.SoftBurst
                : hero.ratio >= 0.4 ? MaterialShape.Sunny
                : MaterialShape.Cookie4Sided

            Behavior on color {
                CAnim {}
            }

            StyledText {
                id: usageLabel

                anchors.bottom: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                theme: root.theme
                role: "bodySmall"
                color: root.theme.onSurfaceVariantColour
                text: "Usage"
            }

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

    // Storage: used-percentage ring beside the absolute figures, as in
    // StorageCard.qml. The Linux card also carries a disk picker; Windows
    // exposes a single system volume here, so there is nothing to pick.
    component StorageCard: Card {
        radius: root.theme.roundingExtraLargeIncreased

        Row {
            anchors.centerIn: parent
            spacing: root.theme.spacingLarge

            CircularProgress {
                id: storageRing

                width: 132
                height: 132
                theme: root.theme
                animate: root.animations
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
                anchors.verticalCenter: parent.verticalCenter
                width: Math.max(storageText.implicitWidth, root.theme.paddingExtraLarge * 2)
                spacing: root.theme.spacingExtraSmall

                StyledText {
                    theme: root.theme
                    role: "titleSmall"
                    weight: Font.DemiBold
                    text: "Stockage"
                }

                StyledText {
                    id: storageText

                    theme: root.theme
                    role: "bodyMedium"
                    color: root.theme.secondary
                    text: System.diskUsedGb.toFixed(0) + " / "
                        + System.diskTotalGb.toFixed(0) + " Go"
                }
            }
        }
    }

    // Network: header, sparkline filling the middle, then download and upload
    // rows pinned to the bottom — NetworkCard.qml.
    component NetworkCard: Card {
        id: network

        radius: root.theme.roundingExtraLarge

        property var downValues: []
        property var upValues: []

        Timer {
            interval: 1000
            running: true
            repeat: true
            triggeredOnStart: true
            onTriggered: {
                let down = network.downValues.slice();
                let up = network.upValues.slice();
                down.push(Math.max(0, System.networkDownKbps));
                up.push(Math.max(0, System.networkUpKbps));
                if (down.length > 60)
                    down.shift();
                if (up.length > 60)
                    up.shift();
                network.downValues = down;
                network.upValues = up;
            }
        }

        readonly property real graphMax: {
            let maximum = 1024;
            for (let i = 0; i < downValues.length; ++i)
                maximum = Math.max(maximum, downValues[i]);
            for (let i = 0; i < upValues.length; ++i)
                maximum = Math.max(maximum, upValues[i]);
            return maximum * 1.15;
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: root.theme.paddingLarge
            anchors.bottomMargin: root.theme.paddingMedium
            spacing: 0

            Row {
                Layout.leftMargin: -root.theme.paddingExtraSmall
                spacing: root.theme.spacingSmall

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
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.topMargin: root.theme.spacingMedium
                Layout.bottomMargin: root.theme.spacingSmall

                Sparkline {
                    anchors.fill: parent
                    values: network.downValues
                    maximum: network.graphMax
                    lineColor: root.theme.tertiary
                }

                Sparkline {
                    anchors.fill: parent
                    values: network.upValues
                    maximum: network.graphMax
                    lineColor: root.theme.secondary
                }

                StyledText {
                    anchors.centerIn: parent
                    visible: network.downValues.length < 2
                    theme: root.theme
                    role: "bodySmall"
                    color: root.theme.outline
                    text: "Collecte des données…"
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: root.theme.spacingSmall

                MaterialIcon {
                    theme: root.theme
                    text: "download"
                    iconSize: 18
                    color: root.theme.tertiary
                }

                StyledText {
                    text: "Téléchargement"
                    theme: root.theme
                    role: "bodySmall"
                    color: root.theme.onSurfaceVariantColour
                }

                Item {
                    Layout.fillWidth: true
                }

                StyledText {
                    theme: root.theme
                    role: "bodySmall"
                    weight: Font.Medium
                    color: root.theme.tertiary
                    text: root.formatRate(System.networkDownKbps)
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: root.theme.spacingSmall

                MaterialIcon {
                    theme: root.theme
                    text: "upload"
                    iconSize: 18
                    color: root.theme.secondary
                }

                StyledText {
                    text: "Envoi"
                    theme: root.theme
                    role: "bodySmall"
                    color: root.theme.onSurfaceVariantColour
                }

                Item {
                    Layout.fillWidth: true
                }

                StyledText {
                    theme: root.theme
                    role: "bodySmall"
                    weight: Font.Medium
                    color: root.theme.secondary
                    text: root.formatRate(System.networkUpKbps)
                }
            }
        }
    }

    // Memory: title above the ring, absolute figures under it — MemoryCard.qml.
    component MemoryCard: Card {
        radius: root.theme.roundingMedium

        Column {
            anchors.centerIn: parent
            spacing: root.theme.spacingExtraSmall

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: root.theme.spacingSmall

                MaterialIcon {
                    anchors.verticalCenter: parent.verticalCenter
                    theme: root.theme
                    text: "memory_alt"
                    iconSize: 20
                    color: root.theme.tertiary
                }

                StyledText {
                    anchors.verticalCenter: parent.verticalCenter
                    theme: root.theme
                    role: "titleSmall"
                    weight: Font.DemiBold
                    text: "Mémoire"
                }
            }

            CircularProgress {
                id: memoryRing

                anchors.horizontalCenter: parent.horizontalCenter
                width: 150
                height: 150
                theme: root.theme
                animate: root.animations
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
                theme: root.theme
                role: "bodyMedium"
                text: System.memoryUsedGb.toFixed(1) + " / "
                    + System.memoryTotalGb.toFixed(1) + " Go"
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: root.theme.spacingMedium

        RowLayout {
            Layout.fillWidth: true
            spacing: root.theme.spacingMedium

            HeroCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 180
                theme: root.theme
                animations: root.animations
                icon: "memory"
                label: "CPU"
                subLabel: System.cpuName
                accent: root.theme.primary
                usage: System.cpuUsage
                usageAvailable: true
            }

            HeroCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 180
                visible: System.gpuUsageAvailable
                theme: root.theme
                animations: root.animations
                icon: "desktop_windows"
                label: "GPU"
                subLabel: System.gpuName !== "" ? System.gpuName : "Aucun GPU détecté"
                accent: root.theme.secondary
                usage: System.gpuUsage
                usageAvailable: System.gpuUsageAvailable
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: root.theme.spacingMedium

            StorageCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                theme: root.theme
            }

            NetworkCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                theme: root.theme
            }

            MemoryCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                theme: root.theme
            }
        }
    }

    // Linux hides the battery tank on non-laptop devices; Windows provides no
    // laptop battery on this machine, so no empty decorative tank is drawn.
}
