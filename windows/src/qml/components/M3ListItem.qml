import QtQuick
import ".."

Item {
    id: root

    required property var theme

    property string icon: ""
    property string iconSource: ""
    property string title: ""
    property string subtitle: ""
    property string trailingText: ""
    property bool selected: false
    signal clicked()

    implicitWidth: 320
    implicitHeight: subtitle !== "" ? 64 : 56
    activeFocusOnTab: enabled
    scale: stateLayer.pressed ? 0.99 : 1

    Behavior on scale {
        Anim {
            type: Anim.FastSpatial
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: root.theme.roundingMedium
        color: root.selected ? root.theme.secondaryContainer : "transparent"

        Behavior on color {
            CAnim {}
        }
    }

    StateLayer {
        id: stateLayer

        anchors.fill: parent
        theme: root.theme
        colour: root.selected ? root.theme.onSecondaryContainerColour : root.theme.onSurfaceColour
        radius: root.theme.roundingMedium
        disabled: !root.enabled
        onClicked: {
            root.forceActiveFocus();
            root.clicked();
        }
    }

    Row {
        anchors.fill: parent
        anchors.leftMargin: root.theme.paddingMedium
        anchors.rightMargin: root.theme.paddingMedium
        spacing: root.theme.spacingMedium

        MaterialIcon {
            visible: root.icon !== "" && root.iconSource === ""
            anchors.verticalCenter: parent.verticalCenter
            theme: root.theme
            text: root.icon
            iconSize: 22
            color: root.selected ? root.theme.onSecondaryContainerColour : root.theme.onSurfaceVariantColour
        }

        Image {
            visible: root.iconSource !== ""
            anchors.verticalCenter: parent.verticalCenter
            width: 24
            height: 24
            source: root.iconSource
            asynchronous: true
            fillMode: Image.PreserveAspectFit
        }

        Column {
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width - (root.icon !== "" ? 34 : 0) - trailing.width - root.theme.spacingMedium
            spacing: 2

            StyledText {
                width: parent.width
                theme: root.theme
                role: "bodyMedium"
                color: root.selected ? root.theme.onSecondaryContainerColour : root.theme.onSurfaceColour
                elide: Text.ElideRight
                text: root.title
            }

            StyledText {
                width: parent.width
                visible: root.subtitle !== ""
                theme: root.theme
                role: "labelSmall"
                color: root.selected ? root.theme.onSecondaryContainerColour : root.theme.onSurfaceVariantColour
                opacity: 0.75
                elide: Text.ElideRight
                text: root.subtitle
            }
        }

        StyledText {
            id: trailing

            anchors.verticalCenter: parent.verticalCenter
            visible: root.trailingText !== ""
            theme: root.theme
            role: "labelSmall"
            color: root.selected ? root.theme.onSecondaryContainerColour : root.theme.onSurfaceVariantColour
            text: root.trailingText
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: root.theme.roundingMedium - 1
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
