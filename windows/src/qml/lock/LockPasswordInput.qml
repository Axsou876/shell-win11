pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Caelestia.Platform 1.0
import "../components"

// Password entry for the lock screen. The plaintext never leaves this field
// except through Lock.authenticate(), which hands it to LogonUserW on a worker
// thread and wipes its own copy afterwards.
Rectangle {
    id: root

    required property var theme
    property real scaleFactor: 1
    property bool showPassword: false

    implicitHeight: Math.round(58 * root.scaleFactor)
    radius: height / 2
    color: root.theme.surfaceContainer
    border.width: 1
    border.color: input.activeFocus
        ? Qt.alpha(root.theme.primary, 0.7)
        : Qt.alpha(root.theme.outlineVariant, 0.5)

    function submit(): void {
        if (input.text.length === 0 || Lock.busy || Lock.blocked)
            return;
        Lock.authenticate(input.text);
    }

    function focusInput(): void {
        input.forceActiveFocus();
    }

    Connections {
        target: Lock

        // A failed check must not leave the rejected password selected or
        // visible; the field is emptied and focus returns to it.
        function onChanged() {
            if (Lock.busy)
                return;
            if (Lock.error !== "" && input.text !== "")
                input.text = "";
        }

        function onBusyChanged() {
            if (!Lock.busy && !Lock.blocked)
                Qt.callLater(() => input.forceActiveFocus());
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: Math.round(6 * root.scaleFactor)
        spacing: root.theme.spacingMedium

        Item {
            Layout.fillHeight: true
            Layout.preferredWidth: height

            MaterialIcon {
                anchors.centerIn: parent
                theme: root.theme
                text: root.showPassword ? "visibility_off" : "lock"
                iconSize: Math.round(19 * root.scaleFactor)
                color: root.theme.onSurfaceVariantColour
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.showPassword = !root.showPassword
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            StyledText {
                anchors.verticalCenter: parent.verticalCenter
                visible: input.text.length === 0
                theme: root.theme
                role: "bodyMedium"
                color: root.theme.outline
                text: Lock.busy ? "Vérification…"
                    : Lock.blocked ? "Trop de tentatives"
                    : "Saisissez votre mot de passe"
            }

            TextInput {
                id: input

                anchors.fill: parent
                verticalAlignment: TextInput.AlignVCenter
                color: root.theme.onSurfaceColour
                font.family: root.theme.bodyFont
                font.pointSize: Math.round(14 * root.scaleFactor)
                selectByMouse: true
                selectionColor: root.theme.primary
                selectedTextColor: root.theme.onPrimaryColour
                echoMode: root.showPassword ? TextInput.Normal : TextInput.Password
                passwordCharacter: "●"
                passwordMaskDelay: 0
                focus: true
                enabled: !Lock.busy && !Lock.blocked
                onAccepted: root.submit()
            }
        }

        M3Button {
            Layout.preferredWidth: Math.round(42 * root.scaleFactor)
            Layout.preferredHeight: Layout.preferredWidth
            theme: root.theme
            icon: Lock.busy ? "hourglass_top" : "arrow_forward"
            variant: input.text.length > 0 ? "filled" : "tonal"
            enabled: input.text.length > 0 && !Lock.busy && !Lock.blocked
            onClicked: root.submit()
        }
    }
}
