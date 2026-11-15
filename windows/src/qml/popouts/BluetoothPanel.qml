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

    // Address awaiting a confirmation click before it is unpaired.
    property string confirmUnpair: ""

    implicitWidth: theme.popoutBluetoothWidth
    implicitHeight: content.implicitHeight

    function activate(device) {
        if (!device.paired) {
            Bluetooth.pairDevice(device.address);
            return;
        }
        // Removing a pairing cannot be undone from the shell: being in range
        // again does not re-pair, the device must be put back in pairing mode.
        // A click on a paired row therefore only arms the action, and the very
        // same row performs it on the second click.
        if (root.confirmUnpair === device.address) {
            root.confirmUnpair = "";
            Bluetooth.unpairDevice(device.address);
        } else {
            root.confirmUnpair = device.address;
            confirmTimer.restart();
        }
    }

    Timer {
        id: confirmTimer

        interval: 4000
        onTriggered: root.confirmUnpair = ""
    }

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
                : Bluetooth.enabled ? "Activé" : I18n.t("Désactivé")
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
                        : Bluetooth.enabled ? I18n.t("Bluetooth activé") : I18n.t("Bluetooth désactivé")
                }

                StyledText {
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.onSurfaceVariantColour
                    text: Bluetooth.known
                        ? I18n.t("État fourni par Windows") : I18n.t("État inconnu")
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

        RowLayout {
            width: parent.width
            spacing: root.theme.spacingSmall

            StyledText {
                Layout.fillWidth: true
                theme: root.theme
                role: "labelSmall"
                color: root.theme.onSurfaceVariantColour
                text: Bluetooth.scanning ? "Recherche en cours…"
                    : Bluetooth.devices.length > 0
                        ? Bluetooth.devices.length + " appareil(s)"
                        : "Aucun appareil connu"
            }

            M3Button {
                theme: root.theme
                text: "Rechercher"
                icon: "bluetooth_searching"
                variant: "text"
                enabled: Bluetooth.enabled && !Bluetooth.scanning
                onClicked: Bluetooth.scanDevices()
            }
        }

        // The popout height follows its content, so a long device list would
        // otherwise push the panel past the bottom of the screen. Cap it and
        // let the list scroll instead.
        Flickable {
            id: deviceScroll

            width: parent.width
            height: Math.min(contentHeight, 240)
            contentWidth: width
            contentHeight: deviceList.height
            clip: true
            interactive: contentHeight > height
            boundsBehavior: Flickable.StopAtBounds

            Column {
                id: deviceList

                width: deviceScroll.width
                spacing: root.theme.spacingExtraSmall

                Repeater {
                    model: Bluetooth.devices

                    delegate: M3ListItem {
                        required property var modelData

                        width: deviceList.width
                        theme: root.theme
                        icon: modelData.connected ? "bluetooth_connected" : "bluetooth"
                        title: modelData.name
                        subtitle: modelData.address
                            + (modelData.paired
                                ? " · " + I18n.t("appairé") : " · " + I18n.t("non appairé"))
                        trailingText: !modelData.paired ? I18n.t("Appairer")
                            : root.confirmUnpair === modelData.address
                                ? I18n.t("Confirmer ?") : I18n.t("Dissocier")
                        onClicked: root.activate(modelData)
                    }
                }
            }
        }

        M3Button {
            theme: root.theme
            text: I18n.t("Paramètres Bluetooth")
            icon: "settings"
            variant: "text"
            onClicked: Qt.openUrlExternally("ms-settings:bluetooth")
        }
    }
}
