import QtQuick

Item {
    id: root

    property real radius
    property real topLeftRadius: radius
    property real topRightRadius: radius
    property real bottomRightRadius: radius
    property real bottomLeftRadius: radius
    property color color: "transparent"
    default property alias contentData: content.data

    Item {
        id: content

        anchors.fill: parent
    }

    // live must stay false: a live source re-renders its item into a texture
    // every frame and, because these clips exist in eagerly built panels (the
    // dashboard, lock screen), it kept every window repainting at the refresh
    // rate while the shell was idle. Without live, the texture is refreshed
    // whenever the content actually changes, which is all a clip mask needs.
    ShaderEffectSource {
        id: source

        hideSource: true
        sourceItem: content
    }

    RoundedMask {
        anchors.fill: parent
        source: source
        size: Qt.vector2d(root.width, root.height)
        radii: Qt.vector4d(root.topLeftRadius, root.topRightRadius, root.bottomRightRadius, root.bottomLeftRadius)
        color: root.color
    }
}

