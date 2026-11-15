pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Caelestia.Platform 1.0
import "../components"

// Notification dock on the lock screen. Cards are read-only here: dismissing
// or acting on a notification is a decision for the session behind the lock.
Rectangle {
    id: root

    required property var theme
    property real scaleFactor: 1

    color: root.theme.surfaceContainer
    radius: root.theme.roundingExtraLarge

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.theme.paddingLarge
        spacing: root.theme.spacingSmall

        StyledText {
            Layout.fillWidth: true
            theme: root.theme
            role: "labelSmall"
            weight: Font.DemiBold
            color: root.theme.onSurfaceVariantColour
            elide: Text.ElideRight
            text: Notification.count > 0
                ? "NOTIFICATIONS · " + Notification.count : "NOTIFICATIONS"
        }

        // The empty state cannot live inside the Flickable: a Flickable
        // positions its children as content, so anchors on them are ignored
        // and the row would be clipped at the content origin.
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Flickable {
                id: scroller

                anchors.fill: parent
                visible: Notification.count > 0
                contentWidth: width
                contentHeight: list.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                interactive: contentHeight > height

                Column {
                    id: list

                    width: scroller.width
                    spacing: root.theme.spacingSmall

                    Repeater {
                        model: Notification.notifications

                        Rectangle {
                            id: card

                            required property var modelData

                            width: list.width
                            height: cardColumn.implicitHeight + root.theme.paddingMedium * 2
                            radius: root.theme.roundingMedium
                            color: root.theme.surfaceContainerHigh

                            Column {
                                id: cardColumn

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
                                        elide: Text.ElideRight
                                        text: card.modelData.app
                                    }

                                    Item {
                                        Layout.fillWidth: true
                                    }

                                    TimeAgo {
                                        theme: root.theme
                                        role: "labelSmall"
                                        color: root.theme.onSurfaceVariantColour
                                        timestamp: card.modelData.timestamp
                                    }
                                }

                                StyledText {
                                    width: parent.width
                                    theme: root.theme
                                    role: "bodySmall"
                                    weight: Font.Medium
                                    elide: Text.ElideRight
                                    text: card.modelData.title
                                }

                                StyledText {
                                    width: parent.width
                                    visible: text !== ""
                                    theme: root.theme
                                    role: "labelSmall"
                                    color: root.theme.onSurfaceVariantColour
                                    wrapMode: Text.WordWrap
                                    maximumLineCount: 2
                                    elide: Text.ElideRight
                                    text: card.modelData.body
                                }
                            }
                        }
                    }
                }
            }

            Row {
                anchors.centerIn: parent
                spacing: root.theme.spacingSmall
                visible: Notification.count === 0

                MaterialIcon {
                    anchors.verticalCenter: parent.verticalCenter
                    theme: root.theme
                    text: "notifications_off"
                    iconSize: 18
                    color: root.theme.onSurfaceVariantColour
                }

                StyledText {
                    anchors.verticalCenter: parent.verticalCenter
                    theme: root.theme
                    role: "bodySmall"
                    color: root.theme.onSurfaceVariantColour
                    text: I18n.t("Aucune notification")
                }
            }
        }
    }
}
