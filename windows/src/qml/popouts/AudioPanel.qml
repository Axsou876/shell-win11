import QtQuick
import Caelestia.Platform 1.0
import ".."
import "../components"

Item {
    id: root

    required property var theme
    required property var controller

    implicitWidth: theme.popoutAudioWidth
    implicitHeight: content.implicitHeight

    Column {
        id: content

        width: parent.width
        spacing: root.theme.spacingMedium

        PanelHeader {
            width: parent.width
            theme: root.theme
            title: "Audio"
            subtitle: Audio.present ? Audio.deviceName : "Aucune sortie audio"
            onClose: root.controller.closePanel()
        }

        Rectangle {
            width: parent.width
            height: 1
            color: root.theme.outlineVariant
            opacity: 0.6
        }

        Row {
            width: parent.width
            spacing: root.theme.spacingMedium

            MaterialIcon {
                anchors.verticalCenter: parent.verticalCenter
                theme: root.theme
                text: Audio.muted ? "volume_off" : Audio.volume >= 50 ? "volume_up"
                    : Audio.volume > 0 ? "volume_down" : "volume_mute"
                iconSize: 34
                color: root.theme.primary
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter

                StyledText {
                    theme: root.theme
                    role: "bodyLarge"
                    pointSize: 28
                    weight: Font.DemiBold
                    text: Audio.known ? Audio.volume + " %" : "N/D"
                }

                StyledText {
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.onSurfaceVariantColour
                    text: Audio.muted ? "Muet" : "Volume de sortie"
                }
            }

            Item { width: parent.width - 210; height: 1 }

            M3Button {
                anchors.verticalCenter: parent.verticalCenter
                theme: root.theme
                icon: Audio.muted ? "volume_off" : "volume_up"
                variant: "tonal"
                implicitWidth: 44
                implicitHeight: 40
                enabled: Audio.present
                onClicked: Audio.toggleMute()
            }
        }

        M3Slider {
            id: slider

            width: parent.width
            theme: root.theme
            value: Audio.volume / 100
            enabled: Audio.present
            onMoved: Audio.setVolumePercent(Math.round(value * 100))
        }

        Binding {
            target: slider
            property: "value"
            value: Audio.volume / 100
            when: !slider.dragging
        }

        Row {
            width: parent.width
            spacing: root.theme.spacingSmall

            StyledText {
                theme: root.theme
                role: "labelSmall"
                color: root.theme.onSurfaceVariantColour
                text: "0"
            }

            Item { width: parent.width - 60; height: 1 }

            StyledText {
                theme: root.theme
                role: "labelSmall"
                color: root.theme.onSurfaceVariantColour
                text: "100"
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: root.theme.outlineVariant
            opacity: 0.6
        }

        StyledText {
            width: parent.width
            theme: root.theme
            role: "labelSmall"
            color: root.theme.onSurfaceVariantColour
            wrapMode: Text.WordWrap
            text: Audio.present
                ? "Périphérique de sortie : " + Audio.deviceName
                : "Windows ne signale aucun périphérique de sortie actif."
        }

        M3Button {
            theme: root.theme
            text: "Paramètres audio"
            icon: "settings"
            variant: "text"
            onClicked: Qt.openUrlExternally("ms-settings:sound")
        }
    }
}
