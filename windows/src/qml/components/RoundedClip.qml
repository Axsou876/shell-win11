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

    // The source has to be live. The texture only follows the content item
    // itself, while the covers and avatars that get clipped live in child
    // items that load asynchronously — without live, the clip kept the empty
    // texture from before the image arrived.
    ShaderEffectSource {
        id: source

        hideSource: true
        live: true
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

