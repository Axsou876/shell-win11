import QtQuick
import QtQuick.Layouts
import Caelestia.Platform 1.0
import ".."
import "../components"

Item {
    id: root

    required property var theme
    required property var controller

    property string errorText: ""

    implicitWidth: theme.popoutBluetoothWidth
    implicitHeight: content.implicitHeight

    Connections {
        target: Bluetooth

        function onToggleFailed(message) {
            root.errorText = message;
            errorTimer.restart();
        }
    }

    Timer {
        id: errorTimer

        interval: 5000
        onTriggered: root.errorText = ""
    }

    Column {
        id: content

        width: parent.width
        spacing: root.theme.spacingMedium

        PanelHeader {
            width: parent.width
            theme: root.theme
            title: "Bluetooth"
            subtitle: !Bluetooth.present ? "Adaptateur absent"
                : Bluetooth.enabled ? "Activé" : "Désactivé"
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
                text: Bluetooth.enabled ? "bluetooth" : "bluetooth_disabled"
                iconSize: 34
                color: Bluetooth.enabled ? root.theme.primary
                    : root.theme.onSurfaceVariantColour
            }

            Column {
                Layout.alignment: Qt.AlignVCenter

                StyledText {
                    theme: root.theme
                    role: "bodyLarge"
                    weight: Font.DemiBold
                    text: !Bluetooth.present ? "Aucun adaptateur Bluetooth"
                        : Bluetooth.enabled ? "Bluetooth activé" : "Bluetooth désactivé"
                }

                StyledText {
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.onSurfaceVariantColour
                    text: Bluetooth.known
                        ? "État fourni par Windows" : "État inconnu"
                }
            }

            Item { Layout.fillWidth: true }

            M3Switch {
                id: toggle

                Layout.alignment: Qt.AlignVCenter
                theme: root.theme
                checked: Bluetooth.enabled
                enabled: Bluetooth.present && Bluetooth.known
                onToggled: checked => Bluetooth.setEnabled(checked)
            }

            Binding {
                target: toggle
                property: "checked"
                value: Bluetooth.enabled
                when: !toggle.pressed
            }
        }

        StyledText {
            width: parent.width
            theme: root.theme
            role: "labelSmall"
            color: root.theme.error
            wrapMode: Text.WordWrap
            visible: root.errorText !== ""
            text: root.errorText
        }

        StyledText {
            width: parent.width
            theme: root.theme
            role: "labelSmall"
            color: root.theme.onSurfaceVariantColour
            wrapMode: Text.WordWrap
            text: "La liste des appareils associés n'est pas encore exposée. "
                + "Windows gère l'appairage dans ses paramètres."
        }

        M3Button {
            theme: root.theme
            text: "Paramètres Bluetooth"
            icon: "settings"
            variant: "text"
            onClicked: Qt.openUrlExternally("ms-settings:bluetooth")
        }
    }
}
