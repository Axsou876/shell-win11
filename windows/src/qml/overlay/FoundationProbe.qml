import QtQuick
import "../components"

// Temporary neutral content for the popout surface. It deliberately contains
// no service data or fake switch: step 2 replaces it with the real panels.
Item {
    id: root

    required property var theme
    required property var controller

    property string panelName: ""

    readonly property string panelTitle: {
        switch (panelName) {
        case "network":
            return "Réseau";
        case "bluetooth":
            return "Bluetooth";
        case "audio":
            return "Audio";
        case "battery":
            return "Batterie";
        default:
            return "Surface de popout";
        }
    }

    implicitWidth: theme.popoutWidthFor(panelName)
    implicitHeight: column.implicitHeight

    Column {
        id: column

        width: parent.width
        spacing: root.theme.spacingMedium

        Item {
            width: parent.width
            height: 40

            Column {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                spacing: 1

                StyledText {
                    theme: root.theme
                    role: "bodyLarge"
                    weight: Font.DemiBold
                    text: root.panelTitle
                }

                StyledText {
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.onSurfaceVariantColour
                    text: "Survol actif — surface fusionnée"
                }
            }

            M3Button {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                theme: root.theme
                icon: "close"
                variant: "text"
                implicitWidth: 40
                implicitHeight: 40
                onClicked: root.controller.closePanel()
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: root.theme.outlineVariant
            opacity: 0.6
        }

        Row {
            width: parent.width
            height: 56
            spacing: root.theme.spacingMedium

            MaterialIcon {
                anchors.verticalCenter: parent.verticalCenter
                theme: root.theme
                text: "construction"
                iconSize: 24
                color: root.theme.onSurfaceVariantColour
            }

            StyledText {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - 36
                theme: root.theme
                role: "bodySmall"
                color: root.theme.onSurfaceVariantColour
                wrapMode: Text.WordWrap
                text: "Le contenu métier de ce panneau sera ajouté à l'étape 2."
            }
        }
    }
}
