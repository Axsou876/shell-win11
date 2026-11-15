import QtQuick
import QtQuick.Effects
import Caelestia.Platform 1.0
import ".."
import "../components"
import "../components/WindowIcons.js" as WindowIcons

// Application dock, ported from the behaviour of Caelestia's bar wrapper
// (`modules/bar/BarWrapper.qml`): the surface stays collapsed against the
// screen edge and is revealed by hovering that edge, animated with the same
// spatial/emphasized transitions.
//
// Caelestia has no separate bottom dock — its dock *is* the bar, and the part
// worth porting is the reveal gesture: `shouldBeVisible` whenever the bar is
// hovered, an expanding `implicitWidth` with an `Anim` on show and an
// `Anim.Emphasized` on hide. This is the horizontal equivalent for the bottom
// edge.
Item {
    id: root

    required property var theme
    // Monitor geometry in logical coordinates, so the edge test matches the
    // pointer position Platform reports.
    required property var screenGeometry

    readonly property var windows: AppWindows.windows
    readonly property int pillHeight: 64
    readonly property int entityWidth: 52
    readonly property int bandHeight: 128
    readonly property real edgeThickness: 8

    // Revealed when the pointer touches the bottom edge, and kept open while
    // it stays anywhere in the dock's band.
    property bool revealed: false

    readonly property rect edgeZone: Qt.rect(screenGeometry.x,
        screenGeometry.y + screenGeometry.height - edgeThickness,
        screenGeometry.width, edgeThickness)
    readonly property rect bandZone: Qt.rect(screenGeometry.x,
        screenGeometry.y + screenGeometry.height - bandHeight,
        screenGeometry.width, bandHeight)

    function inside(position, rect) {
        return position.x >= rect.x && position.x <= rect.x + rect.width
            && position.y >= rect.y && position.y <= rect.y + rect.height;
    }

    // 0 = shown, 1 = collapsed against the edge, exactly the two states of the
    // bar wrapper.
    property real collapse: revealed ? 0 : 1

    Behavior on collapse {
        Anim {
            type: root.revealed ? Anim.DefaultSpatial : Anim.Emphasized
        }
    }

    onRevealedChanged: {
        if (Window.window)
            Platform.setWindowInputState(Window.window, !root.revealed, false);
    }

    Component.onCompleted: {
        if (Window.window)
            Platform.setWindowInputState(Window.window, true, false);
    }

    // QCursor::pos() polling rather than WM_INPUT: the raw-input path only
    // reports real device movement (the position it delivered did not even
    // follow the pointer), so the reveal felt tied to a physical mouse and
    // could not be driven at all by a script. 50 ms costs nothing — the timer
    // reads a variable, it does not render.
    Timer {
        interval: 50
        running: true
        repeat: true
        onTriggered: root.updatePointer(Platform.cursorPos())
    }

    function updatePointer(position) {
        if (root.inside(position, root.edgeZone))
            root.revealed = true;
        else if (!(root.revealed && root.inside(position, root.bandZone)))
            root.revealed = false;
    }

    Rectangle {
        id: pill

        anchors.horizontalCenter: parent.horizontalCenter
        y: 10 + root.collapse * (root.pillHeight + 24)
        width: Math.max(row.implicitWidth, emptyHint.implicitWidth + hintSpacing)
            + root.theme.paddingLarge * 2
        height: root.pillHeight
        radius: height / 2
        color: root.theme.surface
        border.width: 1
        border.color: Qt.alpha(root.theme.outlineVariant, 0.35)
        opacity: 1 - root.collapse

        Behavior on width {
            Anim { type: Anim.DefaultSpatial }
        }

        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            blurMax: 20
            shadowBlur: 0.55
            shadowVerticalOffset: 2
            shadowColor: Qt.alpha(root.theme.shadow, root.theme.shadowOpacity * 0.8)
        }

        readonly property int hintSpacing: root.theme.paddingLarge

        Row {
            id: row

            anchors.centerIn: parent
            spacing: root.theme.spacingExtraSmall
            visible: root.windows.length > 0

            Repeater {
                model: root.windows

                delegate: Item {
                    id: entry

                    required property var modelData

                    readonly property bool active: entry.modelData.active
                    readonly property string icon: Platform.windowIcon(entry.modelData.id)

                    width: root.entityWidth
                    height: root.entityWidth

                    Rectangle {
                        anchors.fill: parent
                        radius: 16
                        color: root.theme.layer(root.theme.onSurfaceColour, 0.12)
                        opacity: hover.containsMouse ? 1 : 0

                        Behavior on opacity {
                            Anim { type: Anim.DefaultEffects }
                        }
                    }

                    Image {
                        anchors.centerIn: parent
                        width: 34
                        height: 34
                        source: entry.icon
                        sourceSize.width: 68
                        sourceSize.height: 68
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        visible: status === Image.Ready
                    }

                    // Processes without an extractable icon fall back to the
                    // same category glyph the workspace pills use.
                    MaterialIcon {
                        anchors.centerIn: parent
                        visible: entry.icon === ""
                        theme: root.theme
                        text: WindowIcons.iconFor(entry.modelData.app)
                        iconSize: 26
                        color: entry.active
                            ? root.theme.onSurfaceColour : root.theme.onSurfaceVariantColour
                    }

                    // Running indicator: a wide bar under the focused window,
                    // a small dot under the others.
                    Rectangle {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 2
                        width: entry.active ? 16 : 5
                        height: 3
                        radius: 2
                        color: entry.active ? root.theme.onSurfaceColour : root.theme.outline

                        Behavior on width {
                            Anim { type: Anim.FastSpatial }
                        }
                    }

                    MouseArea {
                        id: hover

                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: AppWindows.focusWindow(entry.modelData.id)
                    }
                }
            }
        }

        Row {
            id: emptyHint

            anchors.centerIn: parent
            spacing: root.theme.spacingSmall
            visible: root.windows.length === 0

            MaterialIcon {
                anchors.verticalCenter: parent.verticalCenter
                theme: root.theme
                text: "apps"
                iconSize: 22
                color: root.theme.onSurfaceVariantColour
                opacity: 0.6
            }

            StyledText {
                anchors.verticalCenter: parent.verticalCenter
                theme: root.theme
                role: "bodySmall"
                color: root.theme.onSurfaceVariantColour
                opacity: 0.7
                text: "Aucune application"
            }
        }
    }
}
