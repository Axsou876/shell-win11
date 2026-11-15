import QtQuick
import QtQuick.Layouts
import Caelestia.Platform 1.0
import ".."
import "../components"

// System page: the shell's own switches plus the handful of Windows settings it
// cannot change itself, which are handed to Settings with a deep link.
Flickable {
    id: root

    required property var theme
    required property var controller

    contentWidth: width
    contentHeight: content.implicitHeight
    clip: true
    boundsBehavior: Flickable.StopAtBounds

    // Persisted rendering mode. `shellSetting()` is an invokable without a
    // notifying dependency, so the store's signal is what refreshes this —
    // otherwise the switch's Binding would put the old value back after a
    // click.
    property bool smoothRefresh: Platform.shellSetting("refresh/smooth", false)

    Connections {
        target: Platform

        function onShellSettingChanged(key) {
            if (key === "refresh/smooth")
                root.smoothRefresh = Platform.shellSetting(key, false);
        }
    }

    // M3Switch writes `checked` itself when clicked, which would remove an
    // inline binding; the Binding element survives that and still refreshes the
    // switch when the service changes the value from elsewhere.
    component ToggleRow: Item {
        id: toggle

        required property var theme
        property string icon: ""
        property string title: ""
        property string subtitle: ""
        property bool checked: false
        signal toggled(bool checked)

        implicitHeight: subtitle !== "" ? 64 : 56

        Rectangle {
            anchors.fill: parent
            radius: toggle.theme.roundingMedium
            color: "transparent"
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            spacing: 12

            MaterialIcon {
                theme: toggle.theme
                text: toggle.icon
                iconSize: 20
                color: toggle.theme.onSurfaceVariantColour
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                StyledText {
                    theme: toggle.theme
                    role: "bodyMedium"
                    text: toggle.title
                }

                StyledText {
                    Layout.fillWidth: true
                    theme: toggle.theme
                    role: "labelSmall"
                    color: toggle.theme.onSurfaceVariantColour
                    elide: Text.ElideRight
                    visible: toggle.subtitle !== ""
                    text: toggle.subtitle
                }
            }

            M3Switch {
                id: control

                theme: toggle.theme
                onToggled: checked => toggle.toggled(checked)

                Binding {
                    target: control
                    property: "checked"
                    value: toggle.checked
                }
            }
        }

        StateLayer {
            anchors.fill: parent
            theme: toggle.theme
            radius: toggle.theme.roundingMedium
            onClicked: control.toggle()
        }
    }

    ColumnLayout {
        id: content

        width: root.width
        spacing: root.theme.spacingLarge

        StyledText {
            theme: root.theme
            role: "titleSmall"
            text: I18n.t("Profil d'alimentation")
        }

        StyledText {
            Layout.fillWidth: true
            theme: root.theme
            role: "labelSmall"
            color: root.theme.onSurfaceVariantColour
            wrapMode: Text.WordWrap
            text: PowerProfiles.available
                ? I18n.t("Appliqué immédiatement par le service d'alimentation Windows.")
                : "Le service de profils d'alimentation n'est pas disponible sur cette machine."
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            visible: PowerProfiles.available

            Repeater {
                model: PowerProfiles.profiles

                delegate: M3Button {
                    required property var modelData

                    theme: root.theme
                    text: modelData.name
                    icon: modelData.icon
                    variant: modelData.id === PowerProfiles.activeProfileId ? "filled" : "tonal"
                    onClicked: PowerProfiles.setProfile(modelData.id)
                }
            }

            Item {
                Layout.fillWidth: true
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: 4
            height: 1
            color: root.theme.outlineVariant
            opacity: 0.6
        }

        Item {
            Layout.fillWidth: true
            implicitHeight: 20

            StyledText {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                theme: root.theme
                role: "titleSmall"
                text: I18n.t("Luminosité")
            }

            StyledText {
                anchors.right: parent.right
                // The variable-font metrics run a few pixels wider than the
                // reported advance, and the Flickable clips the difference.
                anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                theme: root.theme
                role: "labelSmall"
                color: root.theme.onSurfaceVariantColour
                text: Brightness.available ? Brightness.percent + " %" : "indisponible"
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            MaterialIcon {
                theme: root.theme
                text: "brightness_low"
                iconSize: 20
                color: root.theme.onSurfaceVariantColour
            }

            M3Slider {
                id: brightness

                Layout.fillWidth: true
                theme: root.theme
                enabled: Brightness.available
                onMoved: value => Brightness.setPercent(Math.round(value * 100))

                Binding {
                    target: brightness
                    property: "value"
                    value: Brightness.percent / 100
                }
            }

            MaterialIcon {
                theme: root.theme
                text: "brightness_high"
                iconSize: 20
                color: root.theme.onSurfaceVariantColour
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: 4
            height: 1
            color: root.theme.outlineVariant
            opacity: 0.6
        }

        StyledText {
            theme: root.theme
            role: "titleSmall"
            text: "Comportement"
        }

        // Qt Quick only paints when something changed. This keeps every shell
        // window drawing at the monitor's refresh rate instead — identical
        // pixels, 144 times a second — so it is off by default.
        ToggleRow {
            Layout.fillWidth: true
            theme: root.theme
            icon: "speed"
            title: I18n.t("Rendu à 144 fps")
            subtitle: I18n.t("Redessine le shell au rythme de l'écran même à l'arrêt. Plus fluide, mais consomme la batterie.")
            checked: root.smoothRefresh
            onToggled: checked => Platform.setShellSetting("refresh/smooth", checked)
        }

        ToggleRow {
            Layout.fillWidth: true
            theme: root.theme
            icon: "coffee"
            title: I18n.t("Empêcher la mise en veille")
            subtitle: IdleInhibit.active
                ? "Actif depuis " + IdleInhibit.since
                : "La session peut se verrouiller automatiquement"
            checked: IdleInhibit.active
            onToggled: checked => IdleInhibit.setActive(checked)
        }

        ToggleRow {
            Layout.fillWidth: true
            theme: root.theme
            icon: "do_not_disturb_on"
            title: I18n.t("Ne pas déranger")
            subtitle: I18n.t("Les notifications restent dans l'historique sans être affichées")
            checked: Notification.doNotDisturb
            onToggled: checked => Notification.doNotDisturb = checked
        }

        ToggleRow {
            Layout.fillWidth: true
            theme: root.theme
            icon: "mic_off"
            title: I18n.t("Microphone coupé")
            subtitle: I18n.t("Coupe l'entrée par défaut, comme le bouton du panneau audio")
            checked: Audio.micMuted
            onToggled: Audio.toggleMicMute()
        }

        ToggleRow {
            Layout.fillWidth: true
            theme: root.theme
            icon: "volume_off"
            title: I18n.t("Son coupé")
            subtitle: Audio.deviceName === "" ? I18n.t("Aucun périphérique de lecture") : Audio.deviceName
            checked: Audio.muted
            onToggled: Audio.toggleMute()
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            M3Button {
                theme: root.theme
                text: "Alimentation"
                icon: "battery_saver"
                variant: "tonal"
                onClicked: Qt.openUrlExternally("ms-settings:powersleep")
            }

            M3Button {
                theme: root.theme
                text: "Notifications"
                icon: "notifications"
                variant: "text"
                onClicked: Qt.openUrlExternally("ms-settings:notifications")
            }

            M3Button {
                theme: root.theme
                text: "Son"
                icon: "graphic_eq"
                variant: "text"
                onClicked: Qt.openUrlExternally("ms-settings:sound")
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
