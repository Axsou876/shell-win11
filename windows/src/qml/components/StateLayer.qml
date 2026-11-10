import QtQuick
import ".."

MouseArea {
    id: root

    required property var theme

    property bool disabled: false
    property color colour: theme.onSurfaceColour
    property real radius: Math.min(width, height) / 2
    property real pressX: width / 2
    property real pressY: height / 2
    property real endRadius: Math.min(width, height) / 2

    enabled: !disabled
    hoverEnabled: true
    cursorShape: disabled ? undefined : Qt.PointingHandCursor

    Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: root.colour
        opacity: root.containsMouse && !root.disabled ? 0.08 : 0

        Behavior on opacity {
            Anim {
                type: Anim.DefaultEffects
            }
        }
    }

    Item {
        id: ripple

        x: root.pressX - width / 2
        y: root.pressY - height / 2
        width: root.endRadius * 2
        height: width
        opacity: 0
        scale: 0

        Rectangle {
            anchors.fill: parent
            radius: width / 2
            color: root.colour
        }
    }

    onPressed: event => {
        root.pressX = event.x;
        root.pressY = event.y;
        ripple.scale = 0;
        ripple.opacity = 0.1;
        expandAnim.restart();
    }

    onReleased: fadeAnim.start()
    onCanceled: fadeAnim.start()

    Anim {
        id: expandAnim

        target: ripple
        property: "scale"
        from: 0
        to: 1
        type: Anim.StandardLarge
    }

    Anim {
        id: fadeAnim

        target: ripple
        property: "opacity"
        from: 0.1
        to: 0
        type: Anim.SlowEffects
    }
}

