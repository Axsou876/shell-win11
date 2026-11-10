import QtQuick
import QtQuick.Effects
import ".."

// Flat M3 surface with a soft shadow. The original Linux shell embeds panels
// in a full-screen blob; Windows owns the wallpaper, so panels get their own
// surface instead of faking that backdrop.
Item {
    id: root

    required property var theme

    property real radius: theme.borderRounding
    property real topLeftRadius: radius
    property real topRightRadius: radius
    property real bottomRightRadius: radius
    property real bottomLeftRadius: radius
    property color color: theme.surface

    RectangularShadow {
        width: surface.width
        height: surface.height
        radius: root.radius
        blur: 18
        spread: -2
        offset: Qt.vector2d(0, 6)
        color: Qt.alpha(root.theme.shadow, 0.55)
    }

    Rectangle {
        id: surface

        anchors.fill: parent
        color: root.color
        topLeftRadius: root.topLeftRadius
        topRightRadius: root.topRightRadius
        bottomRightRadius: root.bottomRightRadius
        bottomLeftRadius: root.bottomLeftRadius

        Behavior on color {
            CAnim {}
        }
    }
}
