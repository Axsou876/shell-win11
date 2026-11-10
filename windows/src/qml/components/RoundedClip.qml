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

