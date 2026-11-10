pragma ComponentBehavior: Bound

import QtQuick
import ".."

Item {
    id: root

    required property var theme

    property int activeWorkspace: 1
    property var occupiedWorkspaces: [1, 3]
    property var windowIcons: ({ 1: ["terminal"], 3: ["folder"] })
    property int focusedShape: MaterialShape.Diamond

    readonly property var focusedShapeList: [
        MaterialShape.Slanted, MaterialShape.Oval, MaterialShape.Pill,
        MaterialShape.Triangle, MaterialShape.Arrow, MaterialShape.Diamond,
        MaterialShape.Pentagon, MaterialShape.Gem, MaterialShape.VerySunny,
        MaterialShape.Sunny, MaterialShape.Cookie4Sided, MaterialShape.Cookie6Sided,
        MaterialShape.Cookie7Sided, MaterialShape.Cookie9Sided,
        MaterialShape.Cookie12Sided, MaterialShape.Clover4Leaf,
        MaterialShape.SoftBurst, MaterialShape.Ghostish
    ]

    readonly property var shapeEasing: [0.38, 1.21, 0.22, 1, 1, 1]
    readonly property real indicatorY: content.y + rowY(activeWorkspace)
    readonly property real indicatorHeight: rowHeight(activeWorkspace)

    implicitWidth: theme.barInnerWidth
    implicitHeight: layout.implicitHeight + 8

    function rowHeight(ws) {
        const n = (windowIcons[ws] ?? []).length;
        return 32 + (n > 0 ? n * 20 - 2 + 4 : 0);
    }

    function rowY(ws) {
        let y = 0;
        for (let i = 1; i < ws; ++i)
            y += rowHeight(i) + 4;
        return y;
    }

    function randomShape() {
        return focusedShapeList[Math.floor(Math.random() * focusedShapeList.length)];
    }

    onActiveWorkspaceChanged: focusedShape = randomShape()

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: root.theme.surfaceContainer
    }

    Rectangle {
        id: indicator

        x: content.x
        y: root.indicatorY
        width: content.width
        height: root.indicatorHeight
        radius: width / 2
        color: root.theme.primary

        Behavior on y {
            Anim {}
        }

        Behavior on height {
            Anim {}
        }
    }

    Item {
        id: content

        anchors.fill: parent
        anchors.margins: 4

        Column {
            id: layout

            width: parent.width
            spacing: 4

            Repeater {
                id: repeater

                model: 5

                delegate: Item {
                    id: row

                    required property int index

                    readonly property int ws: index + 1
                    readonly property bool active: root.activeWorkspace === ws
                    readonly property bool occupied: root.occupiedWorkspaces.indexOf(ws) !== -1
                    readonly property var icons: root.windowIcons[ws] ?? []

                    width: layout.width
                    height: 32 + (icons.length > 0 ? icons.length * 20 - 2 + 4 : 0)

                    MaterialShape {
                        id: shape

                        anchors.horizontalCenter: parent.horizontalCenter
                        implicitSize: 32
                        shape: row.active
                            ? root.focusedShape
                            : row.occupied
                                ? MaterialShape.Square
                                : MaterialShape.Circle
                        color: row.active
                            ? root.theme.onPrimaryColour
                            : row.occupied
                                ? root.theme.onSurfaceColour
                                : root.theme.outlineVariant
                        scale: row.active ? 2 / 3 : row.occupied ? 1 / 3 : 1 / 4
                        animationDuration: 500
                        animationEasing.type: Easing.BezierSpline
                        animationEasing.bezierCurve: root.shapeEasing

                        Behavior on color {
                            CAnim {}
                        }

                        Behavior on scale {
                            Anim {}
                        }
                    }

                    Column {
                        anchors.top: shape.bottom
                        anchors.topMargin: -2
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: 0
                        visible: row.icons.length > 0

                        Repeater {
                            model: row.icons

                            MaterialIcon {
                                required property var modelData

                                theme: root.theme
                                text: modelData
                                iconSize: 20
                                grade: 0
                                fill: 0
                                color: row.active ? root.theme.onPrimaryColour : root.theme.onSurfaceVariantColour
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.activeWorkspace = row.ws
                    }
                }
            }
        }
    }
}


