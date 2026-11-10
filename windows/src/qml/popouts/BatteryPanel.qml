import QtQuick
import Caelestia.Platform 1.0
import ".."
import "../components"

Item {
    id: root

    required property var theme
    required property var controller

    implicitWidth: theme.popoutBatteryWidth
    implicitHeight: content.implicitHeight

    Column {
        id: content

        width: parent.width
        spacing: root.theme.spacingMedium

        PanelHeader {
            width: parent.width
            theme: root.theme
            title: "Batterie"
            subtitle: Battery.present ? (Battery.onAc ? "Sur secteur" : "Sur batterie")
                : "Aucune batterie"
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
                text: Battery.charging ? "battery_charging_full"
                    : !Battery.present ? "battery_unknown"
                    : Battery.level <= 20 ? "battery_alert" : "battery_full"
                iconSize: 38
                color: Battery.level >= 0 && Battery.level <= 20 && !Battery.onAc
                    ? root.theme.error : root.theme.primary
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter

                StyledText {
                    theme: root.theme
                    role: "bodyLarge"
                    pointSize: 30
                    weight: Font.DemiBold
                    text: Battery.known ? Battery.level + " %" : "N/D"
                }

                StyledText {
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.onSurfaceVariantColour
                    text: Battery.charging ? "En charge"
                        : Battery.onAc ? "Alimenté par le secteur"
                        : Battery.present ? "Décharge" : "Aucune donnée"
                }
            }
        }

        Meter {
            width: parent.width
            theme: root.theme
            value: Math.max(0, Battery.level)
            colour: Battery.level <= 20 && !Battery.onAc
                ? root.theme.error : root.theme.primary
        }

        StyledText {
            width: parent.width
            theme: root.theme
            role: "labelSmall"
            color: root.theme.onSurfaceVariantColour
            wrapMode: Text.WordWrap
            text: Battery.minutesRemaining >= 0
                ? "Autonomie estimée par Windows : "
                    + Math.floor(Battery.minutesRemaining / 60) + " h "
                    + (Battery.minutesRemaining % 60) + " min"
                : "Windows ne fournit pas d'autonomie restante exploitable."
        }

        M3Button {
            theme: root.theme
            text: "Paramètres d'alimentation"
            icon: "settings"
            variant: "text"
            onClicked: Qt.openUrlExternally("ms-settings:batterysaver")
        }
    }
}
