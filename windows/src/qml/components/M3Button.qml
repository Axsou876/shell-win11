import QtQuick
import ".."

Item {
    id: root

    required property var theme

    property string text: ""
    property string icon: ""
    property string variant: "filled"
    property color foreground: variant === "filled" ? theme.onPrimaryColour
        : variant === "tonal" ? theme.onSecondaryContainerColour
        : theme.primary
    signal clicked()

    readonly property color background: variant === "filled" ? theme.primary
        : variant === "tonal" ? theme.secondaryContainer
        : "transparent"

    implicitWidth: Math.max(40, row.implicitWidth + (text !== "" ? 24 : 16))
    implicitHeight: 40
    activeFocusOnTab: enabled
    opacity: enabled ? 1 : 0.38
    scale: stateLayer.pressed ? 0.98 : 1

    Behavior on scale {
        Anim {
            type: Anim.FastSpatial
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: root.theme.roundingLarge
        color: root.background

        Behavior on color {
            CAnim {}
        }
    }

    StateLayer {
        id: stateLayer

        anchors.fill: parent
        theme: root.theme
        colour: root.foreground
        radius: root.theme.roundingLarge
        disabled: !root.enabled
        onClicked: {
            root.forceActiveFocus();
            root.clicked();
        }
    }

    Row {
        id: row

        anchors.centerIn: parent
        spacing: root.theme.spacingSmall

        MaterialIcon {
            visible: root.icon !== ""
            theme: root.theme
            text: root.icon
            iconSize: 20
            color: root.foreground
        }

        StyledText {
            visible: root.text !== ""
            theme: root.theme
            role: "bodyMedium"
            weight: Font.Medium
            color: root.foreground
            text: root.text
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: root.theme.roundingLarge - 1
        color: "transparent"
        border.width: 2
        border.color: root.theme.primary
        opacity: root.activeFocus ? 1 : 0

        Behavior on opacity {
            Anim {
                type: Anim.FastEffects
            }
        }
    }

    Keys.onSpacePressed: root.clicked()
    Keys.onReturnPressed: root.clicked()
    Keys.onEnterPressed: root.clicked()
}
