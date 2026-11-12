import QtQuick
import QtQuick.Layouts
import Caelestia.Platform 1.0
import "../components"

// Current conditions for the configured city, hidden until Open-Meteo has an
// answer so the column never shows an empty card. The service refreshes every
// 15 minutes on its own.
Rectangle {
    id: root

    required property var theme
    property real scaleFactor: 1

    readonly property bool shown: Weather.available

    implicitHeight: shown ? column.implicitHeight + root.theme.paddingLarge * 2 : 0
    visible: shown
    color: root.theme.surfaceContainer
    radius: root.theme.roundingExtraLarge

    ColumnLayout {
        id: column

        anchors.fill: parent
        anchors.margins: root.theme.paddingLarge
        spacing: root.theme.spacingSmall

        RowLayout {
            Layout.fillWidth: true
            spacing: root.theme.spacingMedium

            MaterialIcon {
                theme: root.theme
                text: Weather.available ? Weather.icon : "cloud"
                iconSize: Math.round(34 * root.scaleFactor)
                color: root.theme.primary
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0

                StyledText {
                    theme: root.theme
                    role: "bodyLarge"
                    pointSize: Math.round(22 * root.scaleFactor)
                    weight: Font.DemiBold
                    text: Weather.temperature
                }

                StyledText {
                    Layout.fillWidth: true
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.onSurfaceVariantColour
                    elide: Text.ElideRight
                    text: Weather.description
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: root.theme.spacingMedium

            StyledText {
                Layout.fillWidth: true
                theme: root.theme
                role: "labelSmall"
                color: root.theme.onSurfaceVariantColour
                elide: Text.ElideRight
                text: Weather.city
            }

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
}
