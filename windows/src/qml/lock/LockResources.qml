import QtQuick
import QtQuick.Layouts
import Caelestia.Platform 1.0
import "../components"

// CPU, memory and storage at a glance. Values come from SystemService, which
// reads the real Windows counters; a counter it cannot read stays at -1 and is
// rendered as a dash instead of a made-up number.
Rectangle {
    id: root

    required property var theme
    property real scaleFactor: 1

    implicitHeight: column.implicitHeight + root.theme.paddingLarge * 2
    color: root.theme.surfaceContainer
    radius: root.theme.roundingExtraLarge

    ColumnLayout {
        id: column

        anchors.fill: parent
        anchors.margins: root.theme.paddingLarge
        spacing: root.theme.spacingMedium

        ResourceRow {
            Layout.fillWidth: true
            theme: root.theme
            icon: "speed"
            label: "PROCESSEUR"
            value: System.cpuUsage
        }

        ResourceRow {
            Layout.fillWidth: true
            theme: root.theme
            icon: "memory"
            label: "MÉMOIRE"
            value: System.memoryPercent
        }

        ResourceRow {
            Layout.fillWidth: true
            theme: root.theme
            icon: "hard_drive"
            label: "STOCKAGE"
            value: System.diskPercent
        }
    }

    component ResourceRow: ColumnLayout {
        id: row

        required property var theme
        required property string icon
        required property string label
        required property real value

        spacing: 3

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            MaterialIcon {
                theme: row.theme
                text: row.icon
                iconSize: 15
                color: row.theme.onSurfaceVariantColour
            }

            StyledText {
                Layout.fillWidth: true
                theme: row.theme
                role: "labelSmall"
                weight: Font.DemiBold
                color: row.theme.onSurfaceVariantColour
                elide: Text.ElideRight
                text: row.label
            }

            StyledText {
                theme: row.theme
                role: "labelSmall"
                text: row.value < 0 ? "—" : Math.round(row.value) + " %"
            }
        }

        Meter {
            Layout.fillWidth: true
            theme: row.theme
            value: Math.max(0, row.value)
        }
    }
}
