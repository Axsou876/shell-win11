import QtQuick
import QtQuick.Layouts
import Caelestia.Platform 1.0
import "../components"

// Windows counterpart of the Linux `caelestiafetch.sh` card: one monospace
// block with the same aligned labels. Every value is read from a real service.
Rectangle {
    id: root

    required property var theme
    property real scaleFactor: 1

    readonly property string details: {
        const lines = [];
        lines.push("OS  : " + User.windowsVersion);
        lines.push("HOST: " + User.hostName);
        lines.push("USER: " + User.userName);
        lines.push("UP  : " + root.uptimeLabel(System.uptimeSeconds));
        if (Battery.present && Battery.known)
            lines.push("BATT: " + (Battery.charging ? "(+) " : "") + Battery.level + "%");
        return lines.join("\n");
    }

    function uptimeLabel(seconds) {
        if (seconds <= 0)
            return "—";
        const days = Math.floor(seconds / 86400);
        const hours = Math.floor((seconds % 86400) / 3600);
        const minutes = Math.floor((seconds % 3600) / 60);
        if (days > 0)
            return days + "j " + hours + "h";
        if (hours > 0)
            return hours + "h " + minutes + "min";
        return minutes + " min";
    }

    implicitHeight: column.implicitHeight + root.theme.paddingLarge * 2
    color: root.theme.surfaceContainer
    radius: root.theme.roundingExtraLarge

    ColumnLayout {
        id: column

        anchors.fill: parent
        anchors.margins: root.theme.paddingLarge
        spacing: root.theme.spacingSmall

        RowLayout {
            Layout.fillWidth: true
            spacing: root.theme.spacingSmall

            Rectangle {
                implicitWidth: prompt.implicitWidth + 14
                implicitHeight: prompt.implicitHeight + 6
                radius: root.theme.roundingSmall
                color: root.theme.primary

                StyledText {
                    id: prompt

                    anchors.centerIn: parent
                    theme: root.theme
                    role: "labelSmall"
                    family: root.theme.monoFont
                    weight: Font.DemiBold
                    color: root.theme.onPrimaryColour
                    text: ">"
                }
            }

            StyledText {
                Layout.fillWidth: true
                theme: root.theme
                role: "bodySmall"
                family: root.theme.monoFont
                color: root.theme.onSurfaceColour
                elide: Text.ElideRight
                text: "caelestiafetch.sh"
            }

            Logo {
                theme: root.theme
                implicitWidth: 22
                implicitHeight: 22
            }
        }

        StyledText {
            Layout.fillWidth: true
            theme: root.theme
            role: "bodySmall"
            family: root.theme.monoFont
            color: root.theme.onSurfaceVariantColour
            textFormat: Text.PlainText
            text: root.details
        }
    }
}
