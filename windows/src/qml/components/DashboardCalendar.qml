import QtQuick
import ".."

// Month calendar used by the dashboard home, with today's date highlighted.
Item {
    id: root

    required property var theme

    property date today: new Date()
    readonly property int firstWeekday: (new Date(today.getFullYear(), today.getMonth(), 1).getDay() + 6) % 7
    readonly property int daysInMonth: new Date(today.getFullYear(), today.getMonth() + 1, 0).getDate()

    function dayNumber(index) {
        const day = index - firstWeekday + 1;
        return day >= 1 && day <= daysInMonth ? day : -1;
    }

    function isToday(index) {
        const day = dayNumber(index);
        return day === root.today.getDate();
    }

    implicitWidth: 520
    implicitHeight: 260

    Column {
        anchors.fill: parent
        spacing: root.theme.spacingSmall

        Row {
            width: parent.width

            StyledText {
                theme: root.theme
                role: "bodyLarge"
                weight: Font.DemiBold
                text: Qt.formatDateTime(root.today, "MMMM yyyy")
            }

            Item { width: parent.width - 180; height: 1 }

            StyledText {
                anchors.verticalCenter: parent.verticalCenter
                theme: root.theme
                role: "labelSmall"
                color: root.theme.onSurfaceVariantColour
                text: Qt.formatDateTime(root.today, "dddd d")
            }
        }

        Grid {
            width: parent.width
            columns: 7
            columnSpacing: 0
            rowSpacing: 3

            readonly property real cellWidth: width / 7

            Repeater {
                model: ["Lun", "Mar", "Mer", "Jeu", "Ven", "Sam", "Dim"]

                StyledText {
                    required property string modelData

                    width: parent.cellWidth
                    horizontalAlignment: Text.AlignHCenter
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.onSurfaceVariantColour
                    text: modelData
                }
            }

            Repeater {
                model: 42

                Item {
                    required property int index

                    width: parent.cellWidth
                    height: 25

                    readonly property int day: root.dayNumber(index)
                    readonly property bool current: root.isToday(index)

                    Rectangle {
                        anchors.centerIn: parent
                        width: 24
                        height: 24
                        radius: 12
                        color: parent.current ? root.theme.primary : "transparent"
                    }

                    StyledText {
                        anchors.centerIn: parent
                        theme: root.theme
                        role: "labelSmall"
                        weight: parent.current ? Font.DemiBold : Font.Normal
                        color: parent.day < 0 ? "transparent"
                            : parent.current ? root.theme.onPrimaryColour
                            : root.theme.onSurfaceColour
                        text: parent.day < 0 ? "" : parent.day
                    }
                }
            }
        }
    }

    Timer {
        interval: 60000
        repeat: true
        running: true
        onTriggered: root.today = new Date()
    }
}
