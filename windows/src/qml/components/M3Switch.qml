import QtQuick
import QtQuick.Shapes
import ".."

Item {
    id: root

    required property var theme

    property bool checked: false
    signal toggled(bool checked)

    readonly property bool pressed: mouse.pressed
    readonly property real trackHeight: theme.bodyMediumPointSize + theme.paddingSmall * 2
    readonly property real trackWidth: trackHeight * 1.7

    implicitWidth: trackWidth
    implicitHeight: trackHeight
    activeFocusOnTab: enabled

    function toggle() {
        if (!enabled)
            return;
        checked = !checked;
        toggled(checked);
    }

    Rectangle {
        id: track

        anchors.fill: parent
        radius: height / 2
        color: root.checked ? root.theme.primary : root.theme.surfaceContainerHighest

        Behavior on color {
            CAnim {}
        }
    }

    Rectangle {
        anchors.fill: track
        radius: track.radius
        color: root.theme.onSurfaceColour
        opacity: mouse.containsMouse && root.enabled ? 0.08 : 0

        Behavior on opacity {
            Anim {
                type: Anim.FastEffects
            }
        }
    }

    Rectangle {
        id: thumb

        readonly property real nonAnimWidth: root.pressed ? height * 1.2 : height

        y: (track.height - height) / 2
        x: root.checked ? track.width - width - root.theme.paddingExtraSmall / 2 : root.theme.paddingExtraSmall / 2
        width: nonAnimWidth
        height: track.height - root.theme.paddingExtraSmall
        radius: height / 2
        color: root.checked ? root.theme.onPrimaryColour : root.theme.outline

        Behavior on x {
            Anim {
                type: Anim.FastSpatial
            }
        }

        Behavior on width {
            Anim {
                type: Anim.FastSpatial
            }
        }

        Behavior on color {
            CAnim {}
        }

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: root.checked ? root.theme.primary : root.theme.onSurfaceColour
            opacity: root.pressed ? 0.1 : mouse.containsMouse ? 0.08 : 0

            Behavior on opacity {
                Anim {
                    type: Anim.FastEffects
                }
            }
        }

        Shape {
            id: icon

            anchors.centerIn: parent
            width: height
            height: parent.height

            property bool isChecked: root.checked
            property bool isPressed: root.pressed

            property point start1: isPressed ? Qt.point(width * 0.2, height / 2)
                : isChecked ? Qt.point(width * 0.15, height / 2)
                : Qt.point(width * 0.15, height * 0.15)
            property point end1: isPressed ? (isChecked ? Qt.point(width * 0.4, height / 2) : Qt.point(width * 0.8, height / 2))
                : isChecked ? Qt.point(width * 0.4, height * 0.7)
                : Qt.point(width * 0.85, height * 0.85)
            property point start2: isPressed ? (isChecked ? Qt.point(width * 0.4, height / 2) : Qt.point(width * 0.2, height / 2))
                : isChecked ? Qt.point(width * 0.4, height * 0.7)
                : Qt.point(width * 0.15, height * 0.85)
            property point end2: isPressed ? Qt.point(width * 0.8, height / 2)
                : isChecked ? Qt.point(width * 0.85, height * 0.2)
                : Qt.point(width * 0.85, height * 0.15)

            ShapePath {
                strokeWidth: root.theme.bodyLargePointSize * 0.15
                strokeColor: root.checked ? root.theme.primary : root.theme.surfaceContainerHighest
                fillColor: "transparent"
                capStyle: ShapePath.RoundCap

                startX: icon.start1.x
                startY: icon.start1.y

                PathLine {
                    x: icon.end1.x
                    y: icon.end1.y
                }

                PathMove {
                    x: icon.start2.x
                    y: icon.start2.y
                }

                PathLine {
                    x: icon.end2.x
                    y: icon.end2.y
                }

                Behavior on strokeColor {
                    CAnim {}
                }
            }

            Behavior on start1 {
                PropertyAnimation {
                    duration: root.theme.durationFastSpatial
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: [0.42, 1.67, 0.21, 0.9, 1, 1]
                }
            }

            Behavior on end1 {
                PropertyAnimation {
                    duration: root.theme.durationFastSpatial
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: [0.42, 1.67, 0.21, 0.9, 1, 1]
                }
            }

            Behavior on start2 {
                PropertyAnimation {
                    duration: root.theme.durationFastSpatial
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: [0.42, 1.67, 0.21, 0.9, 1, 1]
                }
            }

            Behavior on end2 {
                PropertyAnimation {
                    duration: root.theme.durationFastSpatial
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: [0.42, 1.67, 0.21, 0.9, 1, 1]
                }
            }
        }
    }

    MouseArea {
        id: mouse

        anchors.fill: parent
        hoverEnabled: true
        enabled: root.enabled
        cursorShape: root.enabled ? Qt.PointingHandCursor : undefined

        onClicked: {
            root.forceActiveFocus();
            root.toggle();
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: -4
        radius: width / 2
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

    Keys.onSpacePressed: root.toggle()
    Keys.onReturnPressed: root.toggle()
    Keys.onEnterPressed: root.toggle()
}
