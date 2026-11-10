import QtQuick
import Caelestia.Platform 1.0
import ".."
import "../components"

Item {
    id: root

    required property var theme

    Row {
        anchors.fill: parent
        anchors.margins: root.theme.paddingLarge
        spacing: root.theme.spacingLarge

        RingGauge {
            width: (parent.width - root.theme.spacingLarge * 2) / 3
            height: parent.height
            theme: root.theme
            value: System.gpuUsageAvailable ? System.gpuUsage : 0
            primaryText: System.gpuUsageAvailable
                ? Math.round(System.gpuUsage) + " %" : "N/D"
            secondaryText: "GPU · utilisation"
            colour: root.theme.primary
        }

        RingGauge {
            width: (parent.width - root.theme.spacingLarge * 2) / 3
            height: parent.height
            theme: root.theme
            value: System.cpuUsage
            primaryText: Math.round(System.cpuUsage) + " %"
            secondaryText: "CPU · utilisation"
            colour: root.theme.secondary
        }

        RingGauge {
            width: (parent.width - root.theme.spacingLarge * 2) / 3
            height: parent.height
            theme: root.theme
            value: System.memoryPercent
            primaryText: System.memoryUsedGb.toFixed(1) + " Go"
            secondaryText: "Mémoire · " + Math.round(System.memoryPercent) + " %"
            colour: root.theme.tertiary
        }
    }

    Row {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: root.theme.paddingLarge
        anchors.rightMargin: root.theme.paddingLarge
        anchors.bottomMargin: root.theme.paddingLarge

        StyledText {
            theme: root.theme
            role: "labelSmall"
            color: root.theme.onSurfaceVariantColour
            text: System.gpuAvailable
                ? System.gpuName + " · " + Math.round(System.gpuMemoryUsedMb) + " / "
                    + Math.round(System.gpuMemoryTotalMb) + " Mo"
                : "Aucun GPU détecté"
        }

        Item { width: parent.width - 500; height: 1 }

        StyledText {
            theme: root.theme
            role: "labelSmall"
            color: root.theme.onSurfaceVariantColour
            text: "Disque " + Math.round(System.diskPercent) + " % · "
                + System.diskUsedGb.toFixed(1) + " / " + System.diskTotalGb.toFixed(1) + " Go"
        }
    }
}
