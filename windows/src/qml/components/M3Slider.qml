import QtQuick
import ".."

Item {
    id: root

    required property var theme

    property real value: 0
    signal moved(real value)

    readonly property real handleWidth: 4
    readonly property real usableWidth: Math.max(0, width - handleWidth)
    readonly property bool dragging: mouse.pressed

    implicitWidth: 200
    implicitHeight: 20
    activeFocusOnTab: enabled

    function clampValue(v) {
        return Math.max(0, Math.min(1, v));
    }

    function setValue(v, notify) {
        const next = clampValue(v);
        if (next === value)
            return;
        value = next;
        if (notify)
            moved(value);
    }

    function valueFromX(x) {
        return clampValue((x - handleWidth / 2) / usableWidth);
    }

    Rectangle {
        id: track

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        height: 12
        radius: height / 2
        color: root.theme.secondaryContainer

        Behavior on color {
            CAnim {}
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        width: handle.x + handle.width / 2
        height: track.height
        radius: track.radius
        color: root.theme.primary

        Behavior on color {
            CAnim {}
        }
    }

    Rectangle {
        id: handle

        x: root.value * root.usableWidth
        anchors.verticalCenter: parent.verticalCenter
        width: root.handleWidth
        height: root.dragging ? 16 : mouse.containsMouse ? 13 : 12
        radius: width / 2
        color: root.theme.primary

        Behavior on x {
            enabled: !root.dragging

            Anim {
                type: Anim.StandardSmall
            }
        }

        Behavior on height {
            Anim {
                type: Anim.FastSpatial
            }
        }

        Behavior on color {
            CAnim {}
        }
    }

    MouseArea {
        id: mouse

        anchors.fill: parent
        preventStealing: true
        enabled: root.enabled
        cursorShape: root.enabled ? Qt.PointingHandCursor : undefined

        onPressed: event => {
            root.forceActiveFocus();
            root.setValue(root.valueFromX(event.x), true);
        }

        onPositionChanged: event => {
            if (pressed)
                root.setValue(root.valueFromX(event.x), true);
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: -4
        radius: height / 2
        color: "transparent"
        border.width: 2
        border.color: root.theme.primary
        opacity: root.activeFocus ? 1 : 0

        Behavior on opacity {
            Anim {
                type: Anim.FastEffects
            }
        }
    }

    Keys.onLeftPressed: root.setValue(root.value - 0.05, true)
    Keys.onRightPressed: root.setValue(root.value + 0.05, true)
    Keys.onDownPressed: root.setValue(root.value - 0.05, true)
    Keys.onUpPressed: root.setValue(root.value + 0.05, true)
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Home)
            root.setValue(0, true);
        else if (event.key === Qt.Key_End)
            root.setValue(1, true);
        else
            return;
        event.accepted = true;
    }
}
