import QtQuick
import Caelestia.Platform 1.0
import ".."
import "../components"

// Notification inbox matching the reference: long-text cards, actions,
// middle-click and horizontal swipe to dismiss.
Item {
    id: root

    required property var theme
    required property var controller

    property real deformScaleX: 1
    property real deformScaleY: 1

    transform: Scale {
        xScale: root.deformScaleX
        yScale: root.deformScaleY
        origin.x: root.width / 2
        origin.y: root.height / 2
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {}
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: root.theme.paddingLarge
        radius: root.theme.roundingLarge
        color: root.theme.surfaceContainerLow
        border.width: 1
        border.color: Qt.alpha(root.theme.outlineVariant, 0.25)

        Column {
            anchors.fill: parent
            anchors.margins: root.theme.paddingLarge
            spacing: root.theme.spacingMedium

            Row {
                width: parent.width
                height: 40

                MaterialIcon {
                    anchors.verticalCenter: parent.verticalCenter
                    theme: root.theme
                    text: "notifications"
                    iconSize: 22
                    color: root.theme.primary
                }

                StyledText {
                    anchors.verticalCenter: parent.verticalCenter
                    theme: root.theme
                    role: "bodyLarge"
                    weight: Font.DemiBold
                    text: "Notifications"
                }

                StyledText {
                    anchors.verticalCenter: parent.verticalCenter
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.onSurfaceVariantColour
                    text: " · " + Notification.count
                }

                Item { width: parent.width - 260; height: 1 }

                M3Button {
                    anchors.verticalCenter: parent.verticalCenter
                    theme: root.theme
                    icon: "add_alert"
                    variant: "text"
                    implicitWidth: 36
                    implicitHeight: 32
                    onClicked: Notification.addTestNotification()
                }

                M3Button {
                    anchors.verticalCenter: parent.verticalCenter
                    theme: root.theme
                    icon: "close"
                    variant: "text"
                    implicitWidth: 36
                    implicitHeight: 32
                    onClicked: root.controller.closePanel()
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: root.theme.outlineVariant
                opacity: 0.5
            }

            ListView {
                id: list

                width: parent.width
                height: parent.height - 62
                model: Notification.notifications
                clip: true
                spacing: root.theme.spacingSmall
                boundsBehavior: Flickable.StopAtBounds

                delegate: Item {
                    id: delegate

                    required property var modelData

                    width: list.width
                    height: card.height + 2

                    Rectangle {
                        id: card

                        width: parent.width
                        height: content.implicitHeight + root.theme.paddingLarge * 2
                        radius: root.theme.roundingLarge
                        color: root.theme.surfaceContainer
                        border.width: 1
                        border.color: Qt.alpha(root.theme.outlineVariant, 0.25)

                        DragHandler {
                            target: card
                            xAxis.enabled: true
                            yAxis.enabled: false

                            onActiveChanged: {
                                if (active)
                                    return;
                                if (Math.abs(card.x) > 100) {
                                    Notification.dismiss(delegate.modelData.id);
                                } else {
                                    card.x = 0;
                                }
                            }
                        }

                        Column {
                            id: content

                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: root.theme.paddingMedium
                            spacing: root.theme.spacingExtraSmall

                            Row {
                                width: parent.width
                                height: 20

                                StyledText {
                                    anchors.verticalCenter: parent.verticalCenter
                                    theme: root.theme
                                    role: "labelSmall"
                                    weight: Font.DemiBold
                                    color: root.theme.primary
                                    text: delegate.modelData.app
                                }

                                Item { width: root.theme.spacingExtraSmall; height: 1 }

                                TimeAgo {
                                    anchors.verticalCenter: parent.verticalCenter
                                    theme: root.theme
                                    role: "labelSmall"
                                    color: root.theme.onSurfaceVariantColour
                                    timestamp: delegate.modelData.timestamp
                                }

                                Item { width: parent.width - 200; height: 1 }

                                MaterialIcon {
                                    anchors.verticalCenter: parent.verticalCenter
                                    theme: root.theme
                                    text: "close"
                                    iconSize: 16
                                    color: root.theme.onSurfaceVariantColour

                                    MouseArea {
                                        anchors.fill: parent
                                        anchors.margins: -8
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: Notification.dismiss(delegate.modelData.id)
                                    }
                                }
                            }

                            StyledText {
                                width: parent.width
                                theme: root.theme
                                role: "bodyMedium"
                                weight: Font.Medium
                                text: delegate.modelData.title
                            }

                            StyledText {
                                width: parent.width
                                theme: root.theme
                                role: "bodySmall"
                                color: root.theme.onSurfaceVariantColour
                                wrapMode: Text.WordWrap
                                maximumLineCount: 3
                                elide: Text.ElideRight
                                text: delegate.modelData.body
                            }

                            Row {
                                spacing: root.theme.spacingSmall
                                visible: delegate.modelData.actions.length > 0

                                Repeater {
                                    model: delegate.modelData.actions

                                    M3Button {
                                        required property string modelData

                                        theme: root.theme
                                        text: modelData
                                        variant: "tonal"
                                        implicitHeight: 30
                                        onClicked: Notification.invokeAction(
                                            delegate.modelData.id, modelData)
                                    }
                                }
                            }
                        }

                        // Middle-click dismissal, matching the reference.
                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.MiddleButton
                            onClicked: Notification.dismiss(delegate.modelData.id)
                        }
                    }
                }
            }
        }

        Column {
            anchors.centerIn: parent
            spacing: root.theme.spacingSmall
            visible: Notification.count === 0

            MaterialIcon {
                anchors.horizontalCenter: parent.horizontalCenter
                theme: root.theme
                text: "notifications_off"
                iconSize: 38
                color: root.theme.onSurfaceVariantColour
                opacity: 0.7
            }

            StyledText {
                anchors.horizontalCenter: parent.horizontalCenter
                theme: root.theme
                role: "bodyMedium"
                color: root.theme.onSurfaceVariantColour
                text: "Aucune notification"
            }

            M3Button {
                anchors.horizontalCenter: parent.horizontalCenter
                theme: root.theme
                text: "Créer une notification de test"
                icon: "add_alert"
                variant: "text"
                onClicked: Notification.addTestNotification()
            }
        }
    }
}
