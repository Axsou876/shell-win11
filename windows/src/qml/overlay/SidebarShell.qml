import QtQuick
import QtQuick.Layouts
import Caelestia.Platform 1.0
import "../components"

// Control Centre dock: real Windows services, favourites, quick actions and
// confirmed session actions. The blob surface is drawn by ScreenSurface.
Item {
    id: root

    required property var theme
    required property var controller
    required property string screenName

    property real deformScaleX: 1
    property real deformScaleY: 1

    transform: Scale {
        xScale: root.deformScaleX
        yScale: root.deformScaleY
        origin.x: root.width / 2
        origin.y: root.height / 2
    }

    property date now: new Date()

    Timer {
        interval: 1000
        repeat: true
        running: true
        onTriggered: root.now = new Date()
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {}
    }

    // Surfaced under the keep-awake row: Windows can refuse the power request,
    // and a silently flipped-back switch would look like a dead control.
    property string idleError: ""

    Connections {
        target: IdleInhibit

        function onActionFailed(message) {
            root.idleError = message;
            idleErrorTimer.restart();
        }
    }

    Timer {
        id: idleErrorTimer
        interval: 5000
        onTriggered: root.idleError = ""
    }

    Rectangle {
        id: dock

        anchors.fill: parent
        anchors.margins: root.theme.paddingLarge
        radius: root.theme.roundingLarge
        color: root.theme.surfaceContainerLow
        border.width: 1
        border.color: Qt.alpha(root.theme.outlineVariant, 0.25)

        // The dock keeps growing as modules land; scroll once the content is
        // taller than the screen instead of pushing the last rows out of view.
        Flickable {
            id: scroller

            anchors.fill: parent
            anchors.margins: root.theme.paddingLarge
            contentWidth: width
            contentHeight: contentColumn.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            interactive: contentHeight > height

            Column {
                id: contentColumn

                width: scroller.width
                spacing: root.theme.spacingLarge

                RowLayout {
                    width: parent.width

                    Column {
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 1

                        StyledText {
                            theme: root.theme
                            role: "bodyLarge"
                            pointSize: 26
                            weight: Font.DemiBold
                            text: Qt.formatDateTime(root.now, "HH:mm")
                        }

                        StyledText {
                            theme: root.theme
                            role: "labelSmall"
                            color: root.theme.onSurfaceVariantColour
                            text: Qt.formatDateTime(root.now, "dddd d MMMM")
                        }
                    }

                    Item { Layout.fillWidth: true }

                    M3Button {
                        Layout.alignment: Qt.AlignVCenter
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
                    opacity: 0.5
                }

                // Notification dock. On Linux this list *is* the sidebar; the
                // Windows sidebar is the control centre, so the dock sits at
                // the top where the notifications panel's toasts land.
                RowLayout {
                    width: parent.width

                    StyledText {
                        theme: root.theme
                        role: "labelSmall"
                        weight: Font.DemiBold
                        color: root.theme.onSurfaceVariantColour
                        text: Notification.count > 0
                            ? "NOTIFICATIONS · " + Notification.count : "NOTIFICATIONS"
                    }

                    Item { Layout.fillWidth: true }

                    M3Button {
                        visible: Notification.count > 0
                        theme: root.theme
                        icon: "clear_all"
                        variant: "text"
                        implicitWidth: 34
                        implicitHeight: 28
                        onClicked: Notification.clear()
                    }
                }

                Column {
                    width: parent.width
                    spacing: root.theme.spacingSmall

                    Repeater {
                        model: Notification.notifications

                        Rectangle {
                            id: notifCard

                            required property var modelData

                            width: parent.width
                            height: notifColumn.implicitHeight + root.theme.paddingMedium * 2
                            radius: root.theme.roundingMedium
                            color: root.theme.surfaceContainer
                            border.width: 1
                            border.color: Qt.alpha(root.theme.outlineVariant, 0.2)

                            Column {
                                id: notifColumn

                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: root.theme.paddingMedium
                                spacing: 2

                                RowLayout {
                                    width: parent.width

                                    StyledText {
                                        theme: root.theme
                                        role: "labelSmall"
                                        weight: Font.DemiBold
                                        color: root.theme.primary
                                        text: notifCard.modelData.app
                                    }

                                    TimeAgo {
                                        Layout.fillWidth: true
                                        Layout.leftMargin: root.theme.spacingExtraSmall
                                        theme: root.theme
                                        role: "labelSmall"
                                        color: root.theme.onSurfaceVariantColour
                                        elide: Text.ElideRight
                                        timestamp: notifCard.modelData.timestamp
                                    }

                                    MaterialIcon {
                                        Layout.alignment: Qt.AlignVCenter
                                        theme: root.theme
                                        text: "close"
                                        iconSize: 14
                                        color: root.theme.onSurfaceVariantColour

                                        MouseArea {
                                            anchors.fill: parent
                                            anchors.margins: -6
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: Notification.dismiss(
                                                notifCard.modelData.id)
                                        }
                                    }
                                }

                                StyledText {
                                    width: parent.width
                                    theme: root.theme
                                    role: "bodyMedium"
                                    weight: Font.Medium
                                    elide: Text.ElideRight
                                    text: notifCard.modelData.title
                                }

                                StyledText {
                                    width: parent.width
                                    visible: text !== ""
                                    theme: root.theme
                                    role: "bodySmall"
                                    color: root.theme.onSurfaceVariantColour
                                    wrapMode: Text.WordWrap
                                    maximumLineCount: 3
                                    elide: Text.ElideRight
                                    text: notifCard.modelData.body
                                }

                                Row {
                                    width: parent.width
                                    spacing: root.theme.spacingSmall
                                    visible: notifCard.modelData.actions.length > 0

                                    Repeater {
                                        model: notifCard.modelData.actions

                                        M3Button {
                                            required property string modelData

                                            theme: root.theme
                                            text: modelData
                                            variant: "tonal"
                                            implicitHeight: 28
                                            onClicked: Notification.invokeAction(
                                                notifCard.modelData.id, modelData)
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Same intent as the Linux dock's empty state.
                    Row {
                        width: parent.width
                        height: 44
                        spacing: root.theme.spacingSmall
                        visible: Notification.count === 0

                        MaterialIcon {
                            anchors.verticalCenter: parent.verticalCenter
                            theme: root.theme
                            text: "check_circle"
                            iconSize: 18
                            color: root.theme.onSurfaceVariantColour
                        }

                        StyledText {
                            anchors.verticalCenter: parent.verticalCenter
                            theme: root.theme
                            role: "bodySmall"
                            color: root.theme.onSurfaceVariantColour
                            text: "Tout est à jour !"
                        }
                    }
                }

                StyledText {
                    theme: root.theme
                    role: "labelSmall"
                    weight: Font.DemiBold
                    color: root.theme.onSurfaceVariantColour
                    text: "ACCÈS RAPIDE"
                }

                Grid {
                    width: parent.width
                    columns: 2
                    columnSpacing: root.theme.spacingSmall
                    rowSpacing: root.theme.spacingSmall

                    readonly property real cellWidth: (width - columnSpacing) / 2

                    M3Button {
                        width: parent.cellWidth
                        theme: root.theme
                        text: "Applications"
                        icon: "apps"
                        variant: "tonal"
                        onClicked: root.controller.togglePanel("launcher", root.screenName, 0)
                    }

                    M3Button {
                        width: parent.cellWidth
                        theme: root.theme
                        text: "Dashboard"
                        icon: "dashboard"
                        variant: "tonal"
                        onClicked: root.controller.togglePanel("dashboard", root.screenName, 0)
                    }

                    M3Button {
                        width: parent.cellWidth
                        theme: root.theme
                        text: "Explorateur"
                        icon: "folder"
                        variant: "text"
                        onClicked: Qt.openUrlExternally("file:///C:/")
                    }

                    M3Button {
                        width: parent.cellWidth
                        theme: root.theme
                        text: "Paramètres"
                        icon: "settings"
                        variant: "text"
                        onClicked: Qt.openUrlExternally("ms-settings:")
                    }
                }

                StyledText {
                    theme: root.theme
                    role: "labelSmall"
                    weight: Font.DemiBold
                    color: root.theme.onSurfaceVariantColour
                    text: "FAVORIS"
                }

                Column {
                    width: parent.width
                    spacing: 2

                    Repeater {
                        model: Apps.favoriteApps.slice(0, 4)

                        M3ListItem {
                            required property var modelData

                            width: parent.width
                            theme: root.theme
                            icon: "apps"
                            iconSource: modelData.icon
                                ? "file:///" + modelData.icon.replace(/\\/g, "/") : ""
                            title: modelData.name
                            onClicked: Apps.launchById(modelData.id)
                        }
                    }

                    StyledText {
                        width: parent.width
                        visible: Apps.favoriteApps.length === 0
                        theme: root.theme
                        role: "labelSmall"
                        color: root.theme.onSurfaceVariantColour
                        wrapMode: Text.WordWrap
                        text: "Ajoutez des favoris avec l'étoile dans le launcher."
                    }
                }

                StyledText {
                    theme: root.theme
                    role: "labelSmall"
                    weight: Font.DemiBold
                    color: root.theme.onSurfaceVariantColour
                    text: "AUDIO"
                }

                RowLayout {
                    width: parent.width
                    spacing: root.theme.spacingSmall

                    MaterialIcon {
                        Layout.alignment: Qt.AlignVCenter
                        theme: root.theme
                        text: Audio.muted ? "volume_off" : "volume_up"
                        iconSize: 22
                        color: root.theme.primary
                    }

                    StyledText {
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 45
                        theme: root.theme
                        role: "bodyMedium"
                        text: Audio.volume + " %"
                    }

                    M3Slider {
                        id: sideVolume

                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        theme: root.theme
                        value: Audio.volume / 100
                        onMoved: Audio.setVolumePercent(Math.round(value * 100))
                    }

                    M3Button {
                        Layout.alignment: Qt.AlignVCenter
                        theme: root.theme
                        icon: Audio.muted ? "volume_off" : "volume_up"
                        variant: "text"
                        implicitWidth: 34
                        implicitHeight: 32
                        onClicked: Audio.toggleMute()
                    }
                }

                Binding {
                    target: sideVolume
                    property: "value"
                    value: Audio.volume / 100
                    when: !sideVolume.dragging
                }

                StyledText {
                    theme: root.theme
                    role: "labelSmall"
                    weight: Font.DemiBold
                    color: root.theme.onSurfaceVariantColour
                    text: "ÉTAT DU SYSTÈME"
                }

                Row {
                    width: parent.width
                    spacing: root.theme.spacingSmall

                    Rectangle {
                        width: (parent.width - root.theme.spacingSmall * 2) / 3
                        height: 72
                        radius: root.theme.roundingMedium
                        color: root.theme.surfaceContainer

                        Column {
                            anchors.centerIn: parent
                            spacing: 2

                            MaterialIcon {
                                anchors.horizontalCenter: parent.horizontalCenter
                                theme: root.theme
                                text: Network.connected ? "wifi" : "wifi_off"
                                iconSize: 22
                                color: root.theme.primary
                            }
                            StyledText {
                                anchors.horizontalCenter: parent.horizontalCenter
                                theme: root.theme
                                role: "labelSmall"
                                text: Network.connected ? "Connecté" : "Hors ligne"
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.controller.togglePanel("network", root.screenName, 0)
                        }
                    }

                    Rectangle {
                        width: (parent.width - root.theme.spacingSmall * 2) / 3
                        height: 72
                        radius: root.theme.roundingMedium
                        color: root.theme.surfaceContainer

                        Column {
                            anchors.centerIn: parent
                            spacing: 2

                            MaterialIcon {
                                anchors.horizontalCenter: parent.horizontalCenter
                                theme: root.theme
                                text: Bluetooth.enabled ? "bluetooth" : "bluetooth_disabled"
                                iconSize: 22
                                color: root.theme.primary
                            }
                            StyledText {
                                anchors.horizontalCenter: parent.horizontalCenter
                                theme: root.theme
                                role: "labelSmall"
                                text: Bluetooth.present
                                    ? (Bluetooth.enabled ? "Activé" : "Désactivé") : "Absent"
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.controller.togglePanel("bluetooth", root.screenName, 0)
                        }
                    }

                    Rectangle {
                        width: (parent.width - root.theme.spacingSmall * 2) / 3
                        height: 72
                        radius: root.theme.roundingMedium
                        color: root.theme.surfaceContainer

                        Column {
                            anchors.centerIn: parent
                            spacing: 2

                            MaterialIcon {
                                anchors.horizontalCenter: parent.horizontalCenter
                                theme: root.theme
                                text: Battery.charging ? "battery_charging_full" : "battery_full"
                                iconSize: 22
                                color: root.theme.primary
                            }
                            StyledText {
                                anchors.horizontalCenter: parent.horizontalCenter
                                theme: root.theme
                                role: "labelSmall"
                                text: Battery.known ? Battery.level + " %" : "N/D"
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.controller.togglePanel("battery", root.screenName, 0)
                        }
                    }
                }

                StyledText {
                    theme: root.theme
                    role: "labelSmall"
                    weight: Font.DemiBold
                    color: root.theme.onSurfaceVariantColour
                    text: "UTILITAIRES"
                }

                // Same intent as the Linux utilities module's "Keep awake"
                // card: a standing request that stops Windows from sleeping
                // or blanking the display while it is on.
                Rectangle {
                    width: parent.width
                    height: 56
                    radius: root.theme.roundingMedium
                    color: IdleInhibit.active ? root.theme.secondaryContainer : "transparent"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: root.theme.paddingMedium
                        anchors.rightMargin: root.theme.paddingMedium
                        spacing: root.theme.spacingMedium

                        MaterialIcon {
                            Layout.alignment: Qt.AlignVCenter
                            theme: root.theme
                            text: "coffee"
                            iconSize: 22
                            color: IdleInhibit.active
                                ? root.theme.onSecondaryContainerColour
                                : root.theme.onSurfaceVariantColour
                        }

                        Column {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                            spacing: 2

                            StyledText {
                                width: parent.width
                                theme: root.theme
                                role: "bodyMedium"
                                color: IdleInhibit.active
                                    ? root.theme.onSecondaryContainerColour
                                    : root.theme.onSurfaceColour
                                text: "Garder éveillé"
                            }

                            StyledText {
                                width: parent.width
                                theme: root.theme
                                role: "labelSmall"
                                color: IdleInhibit.active
                                    ? root.theme.onSecondaryContainerColour
                                    : root.theme.onSurfaceVariantColour
                                opacity: 0.75
                                text: IdleInhibit.active
                                    ? "Actif depuis " + IdleInhibit.since : "Mise en veille autorisée"
                            }
                        }

                        M3Switch {
                            id: idleSwitch

                            Layout.alignment: Qt.AlignVCenter
                            theme: root.theme
                            checked: IdleInhibit.active
                            onToggled: checked => IdleInhibit.setActive(checked)
                        }

                        // The switch updates itself on click, which would sever
                        // a plain binding; re-applying it from a Binding keeps
                        // the control honest when Windows refuses the request.
                        Binding {
                            target: idleSwitch
                            property: "checked"
                            value: IdleInhibit.active
                        }
                    }
                }

                StyledText {
                    width: parent.width
                    visible: root.idleError !== ""
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.error
                    wrapMode: Text.WordWrap
                    text: root.idleError
                }

                Grid {
                    width: parent.width
                    columns: 3
                    columnSpacing: root.theme.spacingSmall
                    rowSpacing: root.theme.spacingSmall

                    readonly property real cellWidth: (width - columnSpacing * 2) / 3

                    QuickToggle {
                        width: parent.cellWidth
                        theme: root.theme
                        icon: Bluetooth.enabled ? "bluetooth" : "bluetooth_disabled"
                        label: "Bluetooth"
                        active: Bluetooth.enabled
                        enabled: Bluetooth.present
                        onToggled: Bluetooth.setEnabled(!Bluetooth.enabled)
                    }

                    QuickToggle {
                        width: parent.cellWidth
                        theme: root.theme
                        icon: Audio.micMuted ? "mic_off" : "mic"
                        label: "Micro"
                        active: !Audio.micMuted
                        onToggled: Audio.toggleMicMute()
                    }

                    QuickToggle {
                        width: parent.cellWidth
                        theme: root.theme
                        icon: Notification.doNotDisturb
                            ? "notifications_off" : "notifications_active"
                        label: "Silencieux"
                        active: Notification.doNotDisturb
                        onToggled: Notification.toggleDoNotDisturb()
                    }
                }

                Item { width: 1; height: 1 }

                M3Button {
                    width: parent.width
                    theme: root.theme
                    text: "Alimentation"
                    icon: "power_settings_new"
                    variant: "tonal"
                    onClicked: root.controller.togglePanel("power", root.screenName, 0)
                }
            }
        }
    }

    // Compact icon + label tile used by the utilities grid. Kept local to the
    // dock: the quick-toggle row of the Linux shell has no other consumer.
    component QuickToggle: Rectangle {
        id: toggle

        property var theme
        property string icon: ""
        property string label: ""
        property bool active: false

        signal toggled()

        implicitHeight: 64
        implicitWidth: 64
        radius: theme.roundingMedium
        color: active ? theme.secondaryContainer : theme.surfaceContainer
        opacity: enabled ? 1 : 0.4

        Column {
            anchors.centerIn: parent
            spacing: 2

            MaterialIcon {
                anchors.horizontalCenter: parent.horizontalCenter
                theme: toggle.theme
                text: toggle.icon
                iconSize: 22
                color: toggle.active
                    ? toggle.theme.onSecondaryContainerColour : toggle.theme.primary
            }

            StyledText {
                anchors.horizontalCenter: parent.horizontalCenter
                theme: toggle.theme
                role: "labelSmall"
                color: toggle.active
                    ? toggle.theme.onSecondaryContainerColour : toggle.theme.onSurfaceColour
                text: toggle.label
            }
        }

        StateLayer {
            anchors.fill: parent
            theme: toggle.theme
            radius: toggle.theme.roundingMedium
            colour: toggle.active
                ? toggle.theme.onSecondaryContainerColour : toggle.theme.onSurfaceColour
            disabled: !toggle.enabled
            onClicked: toggle.toggled()
        }
    }
}
