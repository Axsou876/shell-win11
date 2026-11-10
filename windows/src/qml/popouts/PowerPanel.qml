import QtQuick
import Caelestia.Platform 1.0
import ".."
import "../components"

Item {
    id: root

    required property var theme
    required property var controller

    property string pendingAction: ""
    property string pendingLabel: ""
    property string errorText: ""

    implicitWidth: 340
    implicitHeight: content.implicitHeight

    Connections {
        target: Power

        function onActionFailed(message) {
            root.errorText = message;
        }
    }

    function ask(action, label) {
        pendingAction = action;
        pendingLabel = label;
        errorText = "";
    }

    function confirm() {
        let ok = true;
        if (pendingAction === "lock")
            ok = Power.lock();
        else if (pendingAction === "sleep")
            ok = Power.sleep();
        else if (pendingAction === "restart")
            ok = Power.restart();
        else if (pendingAction === "shutdown")
            ok = Power.shutdown();
        else if (pendingAction === "signout")
            ok = Power.signOut();

        if (ok) {
            pendingAction = "";
            root.controller.closePanel();
        }
    }

    Column {
        id: content

        width: parent.width
        spacing: root.theme.spacingMedium

        PanelHeader {
            width: parent.width
            theme: root.theme
            title: "Alimentation"
            subtitle: "Actions Windows"
            onClose: root.controller.closePanel()
        }

        Rectangle {
            width: parent.width
            height: 1
            color: root.theme.outlineVariant
            opacity: 0.6
        }

        Column {
            width: parent.width
            spacing: root.theme.spacingSmall
            visible: root.pendingAction === ""

            Repeater {
                model: [
                    { key: "lock", icon: "lock", label: "Verrouiller" },
                    { key: "sleep", icon: "bedtime", label: "Mettre en veille" },
                    { key: "signout", icon: "logout", label: "Se déconnecter" },
                    { key: "restart", icon: "restart_alt", label: "Redémarrer" },
                    { key: "shutdown", icon: "power_settings_new", label: "Arrêter" }
                ]

                M3ListItem {
                    required property var modelData

                    width: parent.width
                    theme: root.theme
                    icon: modelData.icon
                    title: modelData.label
                    onClicked: root.ask(modelData.key, modelData.label)
                }
            }
        }

        Column {
            width: parent.width
            spacing: root.theme.spacingMedium
            visible: root.pendingAction !== ""

            StyledText {
                width: parent.width
                theme: root.theme
                role: "bodyLarge"
                weight: Font.DemiBold
                wrapMode: Text.WordWrap
                text: "Confirmer : " + root.pendingLabel + " ?"
            }

            StyledText {
                width: parent.width
                theme: root.theme
                role: "labelSmall"
                color: root.theme.onSurfaceVariantColour
                wrapMode: Text.WordWrap
                text: "Cette action quitte immédiatement la session. "
                    + "Le shell Windows sera arrêté avec elle."
            }

            Row {
                spacing: root.theme.spacingMedium

                M3Button {
                    theme: root.theme
                    text: "Annuler"
                    variant: "text"
                    onClicked: root.pendingAction = ""
                }

                M3Button {
                    theme: root.theme
                    text: "Confirmer"
                    icon: "warning"
                    variant: "filled"
                    onClicked: root.confirm()
                }
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
    }
}
