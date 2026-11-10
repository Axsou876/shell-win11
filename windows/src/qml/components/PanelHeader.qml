import QtQuick
import ".."

Item {
    id: root

    required property var theme

    property string title: ""
    property string subtitle: ""
    signal close()

    implicitWidth: 320
    implicitHeight: 42

    Column {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        spacing: 1

        StyledText {
            theme: root.theme
            role: "bodyLarge"
            weight: Font.DemiBold
            text: root.title
        }

        StyledText {
            visible: root.subtitle !== ""
            theme: root.theme
            role: "labelSmall"
            color: root.theme.onSurfaceVariantColour
            text: root.subtitle
        }
    }

    M3Button {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        theme: root.theme
        icon: "close"
        variant: "text"
        implicitWidth: 40
        implicitHeight: 40
        onClicked: root.close()
    }
}
