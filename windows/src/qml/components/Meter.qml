import QtQuick

// Thin animated progress bar for the dashboard cards.
Item {
    id: root

    required property var theme

    property real value: 0
    property color colour: theme.primary
    implicitHeight: 6

    Rectangle {
        anchors.fill: parent
        radius: height / 2
        color: root.theme.surfaceContainerHighest
    }

    Rectangle {
        width: Math.max(0, Math.min(1, root.value / 100)) * parent.width
        height: parent.height
        radius: height / 2
        color: root.colour

        Behavior on width {
            Anim {
                type: Anim.DefaultSpatial
            }
        }
    }
}
