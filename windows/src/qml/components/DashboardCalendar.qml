import QtQuick

// Compact month calendar rebuilt from the Linux dashboard calendar: chevron
// month navigation, weekend colours and a Sunny-style marker on today.
Item {
    id: root

    required property var theme

    property int monthOffset: 0
    property date baseDate: new Date()
    property real contentOpacity: 1
    property date today: new Date()

    readonly property int year: baseDate.getFullYear()
    readonly property int month: baseDate.getMonth() + monthOffset
    readonly property date cursor: new Date(year, month, 1)
    readonly property int firstWeekday: (new Date(year, month, 1).getDay() + 6) % 7
    readonly property int daysInMonth: new Date(year, month + 1, 0).getDate()

    function dayAt(index) {
        const day = index - firstWeekday + 1;
        const date = new Date(year, month, day);
        return {
            day: date.getDate(),
            inMonth: date.getMonth() === ((month % 12) + 12) % 12,
            today: date.getFullYear() === today.getFullYear()
                && date.getMonth() === today.getMonth()
                && date.getDate() === today.getDate(),
            weekend: date.getDay() === 0 || date.getDay() === 6
        };
    }

    function monthTitle() {
        const text = Qt.formatDate(cursor, "MMMM yyyy");
        return text.charAt(0).toUpperCase() + text.slice(1);
    }

    onMonthOffsetChanged: {
        contentOpacity = 0.35;
        fadeIn.restart();
    }

    NumberAnimation {
        id: fadeIn

        target: root
        property: "contentOpacity"
        from: 0.35
        to: 1
        duration: root.theme.durationFastEffects
        easing.type: Easing.BezierSpline
        easing.bezierCurve: [0.34, 0.8, 0.34, 1, 1, 1]
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.MiddleButton
        onClicked: {
            root.monthOffset = 0;
            root.baseDate = new Date();
            root.today = new Date();
        }
        onWheel: event => {
            if (event.angleDelta.y < 0)
                root.monthOffset += 1;
            else if (event.angleDelta.y > 0)
                root.monthOffset -= 1;
        }
    }

    Column {
        anchors.fill: parent
        spacing: root.theme.spacingExtraSmall
        opacity: root.contentOpacity

        Item {
            id: header

            width: parent.width
            height: 30

            StateLayer {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                width: 28
                height: 28
                radius: 14
                theme: root.theme
                onClicked: root.monthOffset -= 1

                MaterialIcon {
                    anchors.centerIn: parent
                    theme: root.theme
                    text: "chevron_left"
                    iconSize: 18
                    color: root.theme.onSurfaceVariantColour
                }
            }

            StateLayer {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: 28
                height: 28
                radius: 14
                theme: root.theme
                onClicked: root.monthOffset += 1

                MaterialIcon {
                    anchors.centerIn: parent
                    theme: root.theme
                    text: "chevron_right"
                    iconSize: 18
                    color: root.theme.onSurfaceVariantColour
                }
            }

            StyledText {
                anchors.centerIn: parent
                theme: root.theme
                role: "titleSmall"
                weight: Font.DemiBold
                color: root.theme.primary
                text: root.monthTitle()
            }
        }

        Item {
            id: weekdays

            width: parent.width
            height: 16

            Row {
                anchors.fill: parent

                Repeater {
                    model: ["Lun", "Mar", "Mer", "Jeu", "Ven", "Sam", "Dim"]

                    StyledText {
                        required property int index
                        required property string modelData

                        width: weekdays.width / 7
                        horizontalAlignment: Text.AlignHCenter
                        theme: root.theme
                        role: "labelSmall"
                        weight: Font.Medium
                        color: index >= 5 ? root.theme.tertiary : root.theme.onSurfaceColour
                        text: modelData
                    }
                }
            }
        }

        Grid {
            id: grid

            width: parent.width
            height: parent.height - header.height - weekdays.height
                - root.theme.spacingExtraSmall * 2
            columns: 7
            rowSpacing: 1
            columnSpacing: 0

            Repeater {
                model: 42

                Item {
                    id: cell

                    required property int index

                    readonly property var info: root.dayAt(index)

                    width: grid.width / 7
                    height: grid.height / 6

                    Rectangle {
                        anchors.centerIn: parent
                        width: 25
                        height: 25
                        radius: 12.5
                        visible: cell.info.today
                        color: root.theme.primary
                    }

                    StyledText {
                        anchors.centerIn: parent
                        theme: root.theme
                        role: "bodySmall"
                        weight: cell.info.today ? Font.DemiBold : Font.Normal
                        color: cell.info.today ? root.theme.onPrimaryColour
                            : cell.info.weekend ? root.theme.tertiary
                            : root.theme.onSurfaceVariantColour
                        opacity: cell.info.inMonth || cell.info.today ? 1 : 0.4
                        text: cell.info.day
                    }
                }
            }
        }
    }
}
