import QtQuick

Text {
    id: root

    required property var theme

    property real fill: 0
    property real grade: -25
    property int iconSize: 20
    property int weight: Font.Normal
    property bool animate: false

    renderType: Text.NativeRendering
    textFormat: Text.PlainText
    color: theme.onSurfaceColour
    font.family: theme.iconFont
    font.pixelSize: iconSize
    font.weight: weight
    font.variableAxes: ({ "FILL": fill, "GRAD": grade, "opsz": iconSize, "wght": weight })
    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignHCenter

    Behavior on color {
        CAnim {}
    }

    Behavior on text {
        enabled: root.animate

        SequentialAnimation {
            Anim {
                target: root
                property: "opacity"
                to: 0
                type: Anim.FastEffects
            }
            PropertyAction {}
            Anim {
                target: root
                property: "opacity"
                to: 1
                type: Anim.DefaultEffects
            }
        }
    }
}

