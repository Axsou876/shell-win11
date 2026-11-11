pragma ComponentBehavior: Bound

import QtQuick
import Caelestia.Platform 1.0
import ".."
import "../components"

// Real Windows virtual desktops in the Caelestia bar. The service publishes the
// actual Explorer desktop order, active desktop and window mapping; this item
// only draws and forwards switch/create/close/move requests.
Item {
    id: root

    required property var theme

    readonly property bool available: VirtualDesktops.available && VirtualDesktops.count > 0
    readonly property real rowSpacing: 4
    property Item activeRow: null
    property Item dragTargetRow: null
    property string draggingWindowId: ""
    property point dragPoint: Qt.point(0, 0)

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

    implicitWidth: theme.barInnerWidth
    implicitHeight: available ? list.contentHeight + rowSpacing + footer.height + 8 : 0
    visible: available

    Behavior on implicitHeight {
        Anim {}
    }

    function randomShape() {
        return focusedShapeList[Math.floor(Math.random() * focusedShapeList.length)];
    }

    function rowAt(y) {
        for (let i = 0; i < VirtualDesktops.count; ++i) {
            const item = list.itemAtIndex(i);
            if (!item)
                continue;
            if (y >= content.y + list.y + item.y
                && y <= content.y + list.y + item.y + item.height) {
                return item;
            }
        }
        return null;
    }

    function beginWindowDrag(windowId, scenePoint) {
        draggingWindowId = windowId;
        dragPoint = root.mapFromItem(null, scenePoint.x, scenePoint.y);
        dragTargetRow = rowAt(dragPoint.y);
    }

    function updateWindowDrag(scenePoint) {
        dragPoint = root.mapFromItem(null, scenePoint.x, scenePoint.y);
        dragTargetRow = rowAt(dragPoint.y);
    }

    function endWindowDrag() {
        if (draggingWindowId !== "" && dragTargetRow
            && dragTargetRow.id) {
            VirtualDesktops.moveWindow(draggingWindowId, dragTargetRow.id);
        }
        draggingWindowId = "";
        dragTargetRow = null;
    }

    Component.onCompleted: focusedShape = randomShape()

    Connections {
        target: VirtualDesktops

        function onCurrentIdChanged() {
            root.focusedShape = root.randomShape();
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: root.theme.surfaceContainer
    }

    Rectangle {
        id: indicator

        x: content.x
        y: content.y + (root.activeRow ? root.activeRow.y : 0)
        width: content.width
        height: root.activeRow ? root.activeRow.height : 0
        radius: width / 2
        color: root.theme.primary
        visible: root.available && root.activeRow !== null

        Behavior on y {
            Anim { type: Anim.DefaultSpatial }
        }

        Behavior on height {
            Anim { type: Anim.DefaultSpatial }
        }

        Behavior on color {
            CAnim {}
        }
    }

    Item {
        id: content

        anchors.fill: parent
        anchors.margins: 4

        ListView {
            id: list

            width: parent.width
            height: contentHeight
            spacing: root.rowSpacing
            interactive: false
            clip: false
            model: VirtualDesktops

            add: Transition {
                NumberAnimation {
                    property: "opacity"
                    from: 0
                    to: 1
                    duration: 220
                }

                NumberAnimation {
                    property: "scale"
                    from: 0.8
                    to: 1
                    duration: 220
                    easing.type: Easing.OutCubic
                }
            }

            remove: Transition {
                NumberAnimation {
                    property: "opacity"
                    to: 0
                    duration: 180
                }

                NumberAnimation {
                    property: "scale"
                    to: 0.8
                    duration: 180
                    easing.type: Easing.InCubic
                }
            }

            displaced: Transition {
                NumberAnimation {
                    property: "y"
                    duration: 220
                    easing.type: Easing.OutCubic
                }
            }

            delegate: Item {
                id: row

                required property string id
                required property string name
                required property int desktopIndex
                required property bool active
                required property bool occupied
                required property var windows

                readonly property int windowIconCount: Math.min(3, windows.length)

                width: list.width
                height: implicitHeight
                implicitHeight: 32 + (windowIconCount > 0
                    ? windowIconCount * 20 - 2 + 4 : 0)

                Component.onCompleted: {
                    if (active)
                        root.activeRow = row;
                }

                Component.onDestruction: {
                    if (root.activeRow === row)
                        root.activeRow = null;
                }

                onActiveChanged: {
                    if (active)
                        root.activeRow = row;
                    else if (root.activeRow === row)
                        root.activeRow = null;
                }

                Rectangle {
                    anchors.fill: parent
                    radius: width / 2
                    color: root.theme.layer(root.theme.onSurfaceColour, 0.06)
                    opacity: (rowHover.containsMouse
                        || root.dragTargetRow === row) && !row.active ? 1 : 0

                    Behavior on opacity {
                        Anim { type: Anim.DefaultEffects }
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    radius: width / 2
                    color: "transparent"
                    border.width: 1
                    border.color: root.theme.primary
                    visible: root.dragTargetRow === row && !row.active
                }

                MaterialShape {
                    id: shape

                    anchors.horizontalCenter: parent.horizontalCenter
                    y: 0
                    implicitSize: 32
                    shape: row.active ? root.focusedShape
                        : row.occupied ? MaterialShape.Square
                        : MaterialShape.Circle
                    color: row.active ? root.theme.onPrimaryColour
                        : row.occupied ? root.theme.onSurfaceColour
                        : root.theme.outlineVariant
                    scale: row.active ? 2 / 3 : row.occupied ? 1 / 3 : 1 / 4

                    Behavior on color {
                        CAnim {}
                    }

                    Behavior on scale {
                        Anim { type: Anim.FastSpatial }
                    }
                }

                Column {
                    id: windowColumn

                    anchors.top: shape.bottom
                    anchors.topMargin: -2
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 0
                    z: 3

                    Repeater {
                        model: row.windowIconCount

                        Item {
                            id: windowIcon

                            required property int index

                            property string windowId: row.windows[index]
                                ? row.windows[index].id : ""

                            width: 20
                            height: 20

                            DragHandler {
                                id: dragHandler
                                target: null

                                onActiveChanged: {
                                    if (active)
                                        root.beginWindowDrag(windowIcon.windowId,
                                            centroid.scenePosition);
                                    else
                                        root.endWindowDrag();
                                }
                                onCentroidChanged: {
                                    if (active)
                                        root.updateWindowDrag(centroid.scenePosition);
                                }
                            }

                            TapHandler {
                                onTapped: AppWindows.focusWindow(windowIcon.windowId)
                            }

                            MaterialIcon {
                                anchors.centerIn: parent
                                theme: root.theme
                                text: "window"
                                iconSize: 18
                                color: row.active ? root.theme.onPrimaryColour
                                    : root.theme.onSurfaceVariantColour
                            }
                        }
                    }
                }

                MouseArea {
                    id: rowHover

                    anchors.fill: parent
                    z: 1
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.MiddleButton
                    cursorShape: Qt.PointingHandCursor
                    onClicked: mouse => {
                        if (mouse.button === Qt.MiddleButton) {
                            if (VirtualDesktops.count > 1)
                                VirtualDesktops.closeDesktop(row.id);
                        } else if (!row.active) {
                            VirtualDesktops.switchTo(row.id);
                        }
                    }
                }

            }
        }

        // Create / close controls, kept inside the workspace pill.
        Row {
            id: footer

            y: list.height + root.rowSpacing
            width: content.width
            height: 20
            spacing: 0

            StateLayer {
                width: 16
                height: 16
                radius: 8
                theme: root.theme
                onClicked: VirtualDesktops.createDesktop()

                MaterialIcon {
                    anchors.centerIn: parent
                    theme: root.theme
                    text: "add"
                    iconSize: 14
                    color: root.theme.onSurfaceVariantColour
                }
            }

            StateLayer {
                width: 16
                height: 16
                radius: 8
                theme: root.theme
                disabled: VirtualDesktops.count <= 1
                onClicked: VirtualDesktops.closeDesktop(VirtualDesktops.currentId)

                MaterialIcon {
                    anchors.centerIn: parent
                    theme: root.theme
                    text: "remove"
                    iconSize: 14
                    color: VirtualDesktops.count <= 1
                        ? root.theme.outlineVariant : root.theme.onSurfaceVariantColour
                }
            }
        }

    }

    Item {
        id: dragIcon

        x: root.dragPoint.x - 10
        y: root.dragPoint.y - 10
        width: 20
        height: 20
        visible: root.draggingWindowId !== ""
        z: 20

        Rectangle {
            anchors.fill: parent
            radius: width / 2
            color: root.theme.surfaceContainerHighest
            border.width: 1
            border.color: root.theme.primary
        }

        MaterialIcon {
            anchors.centerIn: parent
            theme: root.theme
            text: "window"
            iconSize: 16
            color: root.theme.onSurfaceColour
        }
    }
}
