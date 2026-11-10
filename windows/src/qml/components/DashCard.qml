import QtQuick

// Rounded dashboard card with a compact header and a content slot.
Rectangle {
    id: root

    required property var theme

    property string title: ""
    property string icon: ""
    property int contentHeight: 60
    default property alias content: body.data

    implicitWidth: 200
    implicitHeight: headerRow.height + root.theme.spacingMedium + contentHeight
        + root.theme.paddingLarge * 2
    radius: root.theme.roundingLarge
    color: root.theme.surfaceContainer
    border.width: 1
    border.color: Qt.alpha(root.theme.outlineVariant, 0.25)

    Column {
        id: layout

        anchors.fill: parent
        anchors.margins: root.theme.paddingLarge
        spacing: root.theme.spacingMedium

        Row {
            id: headerRow

            width: parent.width
            height: 20
            spacing: root.theme.spacingSmall

            MaterialIcon {
                anchors.verticalCenter: parent.verticalCenter
                theme: root.theme
                text: root.icon
                iconSize: 18
                color: root.theme.primary
            }

            StyledText {
                anchors.verticalCenter: parent.verticalCenter
                theme: root.theme
                role: "labelSmall"
                weight: Font.DemiBold
                color: root.theme.onSurfaceVariantColour
                text: root.title
            }
        }

        Item {
            id: body

            width: parent.width
            height: root.contentHeight
        }
    }
}


