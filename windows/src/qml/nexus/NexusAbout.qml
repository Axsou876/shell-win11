import QtQuick
import QtQuick.Layouts
import Caelestia.Platform 1.0
import ".."
import "../components"

// About page: the environment the shell is running in. Every value comes from
// a service, so this doubles as the first thing to ask for in a bug report.
Flickable {
    id: root

    required property var theme
    required property var controller

    contentWidth: width
    contentHeight: content.implicitHeight
    clip: true
    boundsBehavior: Flickable.StopAtBounds

    readonly property var rows: [
        { label: "Version", value: Platform.version },
        { label: "Windows", value: User.windowsVersion },
        { label: "Build", value: String(Platform.windowsBuild) },
        { label: "Rendu", value: Platform.rhiBackend },
        { label: "Échelle système", value: Number(Platform.systemScale).toFixed(2) + "×" },
        { label: "Écrans", value: Platform.screens.length + " (" + Platform.primaryScreenName + ")" },
        { label: "Utilisateur", value: User.userName },
        { label: "Machine", value: User.hostName }
    ]

    ColumnLayout {
        id: content

        width: root.width
        spacing: root.theme.spacingLarge

        RowLayout {
            Layout.fillWidth: true
            spacing: 14

            Rectangle {
                Layout.preferredWidth: 52
                Layout.preferredHeight: 52
                radius: 16
                color: root.theme.surfaceContainerHighest

                MaterialIcon {
                    anchors.centerIn: parent
                    theme: root.theme
                    text: "blur_on"
                    iconSize: 26
                    color: root.theme.primary
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                StyledText {
                    theme: root.theme
                    role: "bodyLarge"
                    weight: Font.DemiBold
                    text: "Caelestia Shell"
                }

                StyledText {
                    Layout.fillWidth: true
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.onSurfaceVariantColour
                    wrapMode: Text.WordWrap
                    text: "Portage Windows du shell Quickshell d'origine, en Qt 6 et C++."
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: root.theme.outlineVariant
            opacity: 0.6
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Repeater {
                model: root.rows

                delegate: Item {
                    required property var modelData

                    Layout.fillWidth: true
                    implicitHeight: 34

                    StyledText {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        theme: root.theme
                        role: "bodyMedium"
                        color: root.theme.onSurfaceVariantColour
                        text: modelData.label
                    }

                    StyledText {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        theme: root.theme
                        role: "bodyMedium"
                        font.family: root.theme.monoFont
                        text: modelData.value
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: root.theme.outlineVariant
            opacity: 0.6
        }

        StyledText {
            Layout.fillWidth: true
            theme: root.theme
            role: "labelSmall"
            color: root.theme.onSurfaceVariantColour
            wrapMode: Text.WordWrap
            text: "F8 prévisualise le thème opposé sans toucher aux réglages Windows. "
                + "L'enregistreur d'écran du shell d'origine n'a pas été porté : "
                + "l'outil Capture de Windows le remplace."
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            M3Button {
                theme: root.theme
                text: "À propos de Windows"
                icon: "info"
                variant: "tonal"
                onClicked: Qt.openUrlExternally("ms-settings:about")
            }

            M3Button {
                theme: root.theme
                text: "Notification de test"
                icon: "notifications_active"
                variant: "text"
                onClicked: Notification.addTestNotification()
            }

            Item {
                Layout.fillWidth: true
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
