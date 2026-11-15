import QtQuick
import QtQuick.Layouts
import Caelestia.Platform 1.0
import ".."
import "../components"

Item {
    id: root

    required property var theme
    required property var controller

    property string deviceError: ""

    implicitWidth: theme.popoutAudioWidth
    implicitHeight: content.implicitHeight

    function chooseOutput(id) {
        if (Audio.setDefaultOutput(id))
            root.deviceError = "";
        else
            root.deviceError = I18n.t("Windows a refusé de changer la sortie audio.");
    }

    function chooseInput(id) {
        if (Audio.setDefaultInput(id))
            root.deviceError = "";
        else
            root.deviceError = I18n.t("Windows a refusé de changer l'entrée audio.");
    }

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

        RowLayout {
            width: parent.width
            spacing: root.theme.spacingMedium

            MaterialIcon {
                Layout.alignment: Qt.AlignVCenter
                theme: root.theme
                text: Audio.muted ? "volume_off" : Audio.volume >= 50 ? "volume_up"
                    : Audio.volume > 0 ? "volume_down" : "volume_mute"
                iconSize: 34
                color: root.theme.primary
            }

            Column {
                Layout.alignment: Qt.AlignVCenter

                StyledText {
                    theme: root.theme
                    role: "bodyLarge"
                    pointSize: 28
                    weight: Font.DemiBold
                    text: Audio.known ? Audio.volume + " %" : I18n.t("N/D")
                }

                StyledText {
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.onSurfaceVariantColour
                    text: Audio.muted ? "Muet" : "Volume de sortie"
                }
            }

            Item { Layout.fillWidth: true }

            M3Button {
                Layout.alignment: Qt.AlignVCenter
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

        RowLayout {
            width: parent.width
            spacing: root.theme.spacingSmall

            StyledText {
                theme: root.theme
                role: "labelSmall"
                color: root.theme.onSurfaceVariantColour
                text: "0"
            }

            Item { Layout.fillWidth: true }

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
            text: I18n.t("Périphérique de sortie")
        }

        Column {
            id: outputList

            width: parent.width
            spacing: root.theme.spacingExtraSmall

            Repeater {
                model: Audio.outputs

                delegate: M3ListItem {
                    required property var modelData

                    width: outputList.width
                    theme: root.theme
                    icon: "speaker"
                    title: modelData.name
                    selected: modelData.id === Audio.defaultOutputId
                    onClicked: root.chooseOutput(modelData.id)
                }
            }
        }

        StyledText {
            width: parent.width
            visible: Audio.inputs.length > 0
            theme: root.theme
            role: "labelSmall"
            color: root.theme.onSurfaceVariantColour
            text: I18n.t("Périphérique d'entrée")
        }

        Column {
            id: inputList

            width: parent.width
            spacing: root.theme.spacingExtraSmall

            Repeater {
                model: Audio.inputs

                delegate: M3ListItem {
                    required property var modelData

                    width: inputList.width
                    theme: root.theme
                    icon: "mic"
                    title: modelData.name
                    selected: modelData.id === Audio.defaultInputId
                    onClicked: root.chooseInput(modelData.id)
                }
            }
        }

        StyledText {
            width: parent.width
            visible: root.deviceError !== ""
            theme: root.theme
            role: "labelSmall"
            color: root.theme.error
            wrapMode: Text.WordWrap
            text: root.deviceError
        }

        M3Button {
            theme: root.theme
            text: I18n.t("Paramètres audio")
            icon: "settings"
            variant: "text"
            onClicked: Qt.openUrlExternally("ms-settings:sound")
        }
    }
}
