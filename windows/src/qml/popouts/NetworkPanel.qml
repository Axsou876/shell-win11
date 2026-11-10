import QtQuick
import Caelestia.Platform 1.0
import ".."
import "../components"

Item {
    id: root

    required property var theme
    required property var controller

    implicitWidth: theme.popoutNetworkWidth
    implicitHeight: content.implicitHeight

    Column {
        id: content

        width: parent.width
        spacing: root.theme.spacingMedium

        PanelHeader {
            width: parent.width
            theme: root.theme
            title: "Réseau"
            subtitle: Network.connected ? "Connexion active" : "Aucune connexion"
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
                text: Network.connectionType === "ethernet" ? "cable"
                    : Network.connected ? "wifi" : "wifi_off"
                iconSize: 34
                color: Network.connected ? root.theme.primary
                    : root.theme.onSurfaceVariantColour
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2

                StyledText {
                    theme: root.theme
                    role: "bodyLarge"
                    weight: Font.DemiBold
                    text: {
                        if (!Network.known)
                            return "État indisponible";
                        if (Network.connectionType === "ethernet")
                            return Network.ethernetName || "Ethernet";
                        if (Network.wifiConnected)
                            return Network.wifiSsid;
                        return Network.connected ? "Connecté" : "Déconnecté";
                    }
                }

                StyledText {
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.onSurfaceVariantColour
                    text: Network.connectionType === "wifi"
                        ? "Wi-Fi · signal " + Network.wifiSignal + " %"
                        : Network.connectionType === "ethernet"
                            ? "Ethernet filaire" : "Type : " + Network.connectionType
                }
            }
        }

        Meter {
            width: parent.width
            theme: root.theme
            value: Network.connectionType === "wifi" ? Network.wifiSignal
                : Network.connected ? 100 : 0
            colour: Network.connected ? root.theme.primary
                : root.theme.onSurfaceVariantColour
        }

        Rectangle {
            width: parent.width
            height: 1
            color: root.theme.outlineVariant
            opacity: 0.6
        }

        Row {
            width: parent.width
            spacing: root.theme.spacingLarge

            Column {
                StyledText {
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.onSurfaceVariantColour
                    text: "Descendant"
                }
                StyledText {
                    theme: root.theme
                    role: "bodyMedium"
                    weight: Font.Medium
                    text: root.formatRate(System.networkDownKbps)
                }
            }

            Column {
                StyledText {
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.onSurfaceVariantColour
                    text: "Montant"
                }
                StyledText {
                    theme: root.theme
                    role: "bodyMedium"
                    weight: Font.Medium
                    text: root.formatRate(System.networkUpKbps)
                }
            }
        }

        Row {
            width: parent.width
            spacing: root.theme.spacingSmall

            M3Button {
                theme: root.theme
                text: "Paramètres réseau"
                icon: "settings"
                variant: "text"
                onClicked: Qt.openUrlExternally("ms-settings:network")
            }

            M3Button {
                theme: root.theme
                icon: "refresh"
                variant: "text"
                onClicked: Network.refreshNow()
            }
        }
    }

    function formatRate(kbps) {
        if (kbps >= 1000)
            return (kbps / 1000).toFixed(1) + " Mb/s";
        return Math.round(kbps) + " kb/s";
    }
}
