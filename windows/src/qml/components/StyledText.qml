import QtQuick

Text {
    id: root

    required property var theme

    property bool animate: false
    property string role: "bodySmall"
    property string family: "Google Sans Flex"
    property real pointSize: {
        if (role === "clock")
            return 13.2;
        if (role === "bodyLarge")
            return 16;
        if (role === "bodyMedium")
            return 14;
        if (role === "titleSmall")
            return 14;
        if (role === "labelSmall")
            return 11;
        return 12;
    }
    property int weight: role === "titleSmall" ? Font.Medium : Font.Normal

    renderType: Text.NativeRendering
    textFormat: Text.PlainText
    color: theme.onSurfaceColour
    font.family: family
    font.pointSize: pointSize
    font.weight: weight
    font.variableAxes: family === theme.workspaceFont
        ? ({ "wght": weight })
        : ({ "ROND": 25, "opsz": pointSize, "wght": weight })

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

