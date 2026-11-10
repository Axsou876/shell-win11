pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import ".."
import "../components"

Item {
    id: root

    required property var theme

    property date now: new Date()
    property bool showDate: false

    implicitWidth: theme.barInnerWidth
    implicitHeight: layout.implicitHeight + theme.paddingExtraSmall * 2

    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: root.now = new Date()
    }

    ColumnLayout {
        id: layout

        anchors.centerIn: parent
        spacing: root.theme.spacingExtraSmall

        MaterialIcon {
            Layout.alignment: Qt.AlignHCenter
            theme: root.theme
            text: "calendar_month"
            color: root.theme.tertiary
            iconSize: root.theme.iconSmall
        }

        StyledText {
            Layout.alignment: Qt.AlignHCenter
            theme: root.theme
            role: "clock"
            text: Qt.formatTime(root.now, "hh")
            color: root.theme.tertiary
        }

        StyledText {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: -root.theme.spacingExtraSmall - 4
            theme: root.theme
            role: "clock"
            text: Qt.formatTime(root.now, "mm")
            color: root.theme.tertiary
        }

        Loader {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: -root.theme.spacingExtraSmall - 4
            active: root.showDate
            visible: active

            sourceComponent: ColumnLayout {
                spacing: 0

                StyledText {
                    Layout.alignment: Qt.AlignHCenter
                    theme: root.theme
                    role: "labelSmall"
                    text: Qt.formatDate(root.now, "ddd")
                    color: root.theme.tertiary
                }

                StyledText {
                    Layout.alignment: Qt.AlignHCenter
                    theme: root.theme
                    role: "clock"
                    text: Qt.formatDate(root.now, "d")
                    color: root.theme.tertiary
                }
            }
        }
    }
}


