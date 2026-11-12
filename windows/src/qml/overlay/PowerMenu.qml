import QtQuick
import QtQuick.Effects
import Caelestia.Platform 1.0
import ".."
import "../components"

// Floating vertical session menu anchored to the right edge, matching the
// reference. Destructive actions require a second confirmation.
Item {
    id: root

    required property var theme
    required property var controller

    property string pendingAction: ""
    property string pendingLabel: ""
    property string errorText: ""

    implicitWidth: 76
    // Six 46 px buttons with 14 px gaps plus the same 31 px padding the
    // five-button version used: adding a button must not squeeze the column.
    implicitHeight: 408

    function iconUrl(path) {
        return path ? "file:///" + path.replace(/\\/g, "/") : "";
    }

    function ask(action, label) {
        pendingAction = action;
        pendingLabel = label;
        errorText = "";
    }

    function confirm() {
        let ok = true;
        if (pendingAction === "lock") {
            Lock.requestLock();
            pendingAction = "";
            root.controller.closePanel();
            return;
        } else if (pendingAction === "sleep")
            ok = Power.sleep();
        else if (pendingAction === "signout")
            ok = Power.signOut();
        else if (pendingAction === "restart")
            ok = Power.restart();
        else if (pendingAction === "shutdown")
            ok = Power.shutdown();

        if (ok) {
            pendingAction = "";
            root.controller.closePanel();
        }
    }

    Connections {
        target: Power

        function onActionFailed(message) {
            root.errorText = message;
        }
    }

    component MenuButton: Item {
        id: button

        required property var theme
        property string icon: ""
        property color colour: theme.onSurfaceColour
        property color background: "transparent"
        signal clicked()

        width: 46
        height: 46

        Rectangle {
            anchors.fill: parent
            radius: width / 2
            color: button.background
        }

        StateLayer {
            anchors.fill: parent
            theme: button.theme
            radius: width / 2
            onClicked: button.clicked()
        }

        MaterialIcon {
            anchors.centerIn: parent
            theme: button.theme
            text: button.icon
            iconSize: 22
            color: button.colour
        }
    }

    Rectangle {
        id: panel

        anchors.fill: parent
        radius: 32
        color: root.theme.surfaceContainerHigh
        border.width: 1
        border.color: Qt.alpha(root.theme.outlineVariant, 0.35)
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            blurMax: 18
            shadowOpacity: 0.45
        }

        Column {
            anchors.centerIn: parent
            spacing: 14

            MenuButton {
                theme: root.theme
                icon: "logout"
                onClicked: root.ask("signout", "Se déconnecter")
            }

            MenuButton {
                theme: root.theme
                icon: "power_settings_new"
                colour: root.theme.onPrimaryColour
                background: root.theme.primary
                onClicked: root.ask("shutdown", "Arrêter")
            }

            Item {
                width: 46
                height: 46

                RoundedClip {
                    anchors.fill: parent
                    radius: width / 2

                    Rectangle {
                        anchors.fill: parent
                        color: root.theme.surfaceContainerHighest

                        Image {
                            anchors.fill: parent
                            source: root.iconUrl(User.avatarPath)
                            visible: User.avatarPath !== ""
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                        }

                        MaterialIcon {
                            anchors.centerIn: parent
                            visible: User.avatarPath === ""
                            theme: root.theme
                            text: "account_circle"
                            iconSize: 40
                            color: root.theme.onSurfaceVariantColour
                        }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: Qt.openUrlExternally("ms-settings:yourinfo")
                }
            }

            MenuButton {
                theme: root.theme
                icon: "bedtime"
                onClicked: root.ask("sleep", "Mettre en veille")
            }

            MenuButton {
                theme: root.theme
                icon: "settings"
                // Opens the shell's own settings before falling back to Windows:
                // the panel also offers the ms-settings: links it cannot do itself.
                onClicked: root.controller.openPanel("nexus", root.controller.currentScreenName,
                    Screen.virtualY + Screen.height / 2, "explicit")
            }

            MenuButton {
                theme: root.theme
                icon: "restart_alt"
                onClicked: root.ask("restart", "Redémarrer")
            }
        }
    }

    Rectangle {
        id: confirmation

        x: -218
        y: 92
        width: 200
        height: 132
        radius: root.theme.roundingLarge
        color: root.theme.surfaceContainer
        border.width: 1
        border.color: Qt.alpha(root.theme.outlineVariant, 0.35)
        visible: root.pendingAction !== ""
        opacity: visible ? 1 : 0

        Behavior on opacity {
            Anim { type: Anim.DefaultEffects }
        }

        Column {
            anchors.fill: parent
            anchors.margins: root.theme.paddingMedium
            spacing: root.theme.spacingSmall

            StyledText {
                width: parent.width
                theme: root.theme
                role: "bodyMedium"
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
                text: root.pendingAction === "lock"
                    ? "L’écran de verrouillage Caelestia va s’ouvrir."
                    : "Cette action quitte la session Windows."
            }

            Row {
                spacing: root.theme.spacingSmall

                M3Button {
                    theme: root.theme
                    text: "Annuler"
                    variant: "text"
                    implicitHeight: 32
                    onClicked: root.pendingAction = ""
                }

                M3Button {
                    theme: root.theme
                    text: "Confirmer"
                    icon: "warning"
                    variant: "filled"
                    implicitHeight: 32
                    onClicked: root.confirm()
                }
            }

            StyledText {
                width: parent.width
                visible: root.errorText !== ""
                theme: root.theme
                role: "labelSmall"
                color: root.theme.error
                wrapMode: Text.WordWrap
                text: root.errorText
            }
        }
    }
}
