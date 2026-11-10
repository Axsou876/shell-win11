import QtQuick

ShaderEffect {
    id: root

    property vector2d size: Qt.vector2d(width, height)
    property vector4d radii: Qt.vector4d(0, 0, 0, 0)
    property color color: "transparent"
    property variant source

    vertexShader: "qrc:/src/shaders/roundedmask.vert.qsb"
    fragmentShader: "qrc:/src/shaders/roundedmask.frag.qsb"
}

