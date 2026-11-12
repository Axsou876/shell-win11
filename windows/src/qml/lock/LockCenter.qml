pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Caelestia.Platform 1.0
import "../components"

// Centre column of the lock box: clock, account picture, password field and
// the state/error line, plus the handover to the Windows secure lock.
ColumnLayout {
    id: root

    required property var theme
    required property var overlay
    property real scaleFactor: 1

    property date now: new Date()
    // The shell speaks French everywhere; Qt.formatDateTime would otherwise
    // follow the C locale and print "THURSDAY" in an otherwise French card.
    readonly property var dateLocale: Qt.locale("fr_FR")

    spacing: 8 * root.scaleFactor

    Timer {
        interval: 1000
        repeat: true
        running: true
        onTriggered: root.now = new Date()
    }

    Item {
        Layout.fillHeight: true
    }

    // Clock: hours in the foreground colour, minutes in the secondary one,
    // like the Linux lock's split clock.
    Row {
        Layout.alignment: Qt.AlignHCenter
        spacing: 8 * root.scaleFactor

        StyledText {
            theme: root.theme
            role: "clock"
            pointSize: Math.round(64 * root.scaleFactor)
            weight: Font.DemiBold
            color: root.theme.primary
            text: Qt.formatDateTime(root.now, "HH")
        }

        StyledText {
            theme: root.theme
            role: "clock"
            pointSize: Math.round(64 * root.scaleFactor)
            weight: Font.DemiBold
            color: root.theme.secondary
            text: Qt.formatDateTime(root.now, "mm")
        }
    }

    StyledText {
        Layout.alignment: Qt.AlignHCenter
        theme: root.theme
        role: "titleSmall"
        weight: Font.DemiBold
        color: root.theme.onSurfaceVariantColour
        text: root.now.toLocaleDateString(root.dateLocale, "dddd • d MMM").toUpperCase()
    }

    RoundedClip {
        Layout.alignment: Qt.AlignHCenter
        Layout.preferredWidth: Math.round(84 * root.scaleFactor)
        Layout.preferredHeight: Math.round(84 * root.scaleFactor)
        Layout.topMargin: 18 * root.scaleFactor
        Layout.bottomMargin: 6 * root.scaleFactor
        radius: width / 2
        color: root.theme.surfaceContainerHighest

        Image {
            anchors.fill: parent
            visible: User.avatarPath !== ""
            source: User.avatarPath !== ""
                ? "file:///" + User.avatarPath.replace(/\\/g, "/") : ""
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
        }

        MaterialIcon {
            anchors.centerIn: parent
            visible: User.avatarPath === ""
            theme: root.theme
            text: "person"
            iconSize: Math.round(42 * root.scaleFactor)
            color: root.theme.onSurfaceVariantColour
        }
    }

    LockPasswordInput {
        id: password

        Layout.alignment: Qt.AlignHCenter
        Layout.preferredWidth: Math.round(root.width * 0.9)
        theme: root.theme
        scaleFactor: root.scaleFactor
    }

    // Caps Lock is the single most common cause of a rejected password; the
    // service polls the key state while the screen is up.
    RowLayout {
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: 6 * root.scaleFactor
        Layout.fillWidth: true
        spacing: 6 * root.scaleFactor
        visible: Lock.capsLock && Lock.error === ""

        MaterialIcon {
            theme: root.theme
            text: "keyboard_capslock"
            iconSize: 15
            color: root.theme.onSurfaceVariantColour
        }

        StyledText {
            theme: root.theme
            role: "labelSmall"
            color: root.theme.onSurfaceVariantColour
            text: "Verrouillage majuscules activé"
        }
    }

    // Error and attempt counter. Every failure comes from the Win32 check, so
    // the message says what Windows actually reported.
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 2 * root.scaleFactor
        visible: Lock.error !== ""

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 6 * root.scaleFactor

            MaterialIcon {
                theme: root.theme
                text: "error"
                iconSize: 16
                color: root.theme.onSurfaceColour
            }

            StyledText {
                Layout.maximumWidth: root.width - 40
                theme: root.theme
                role: "bodySmall"
                weight: Font.Medium
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: Lock.error
            }
        }

        StyledText {
            Layout.alignment: Qt.AlignHCenter
            visible: Lock.attempts > 0
            theme: root.theme
            role: "labelSmall"
            color: root.theme.onSurfaceVariantColour
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: "Tentative " + Lock.attempts + " sur " + Lock.maxAttempts
        }
    }

    Item {
        Layout.fillHeight: true
    }

    M3Button {
        Layout.alignment: Qt.AlignHCenter
        theme: root.theme
        text: "Écran de verrouillage Windows"
        icon: "desktop_windows"
        variant: "tonal"
        onClicked: root.overlay.useWindowsLock()
    }

    StyledText {
        Layout.alignment: Qt.AlignHCenter
        Layout.fillWidth: true
        Layout.bottomMargin: 4 * root.scaleFactor
        theme: root.theme
        role: "labelSmall"
        color: root.theme.onSurfaceVariantColour
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        text: "PIN et Windows Hello ne sont pas acceptés ici."
    }
}
