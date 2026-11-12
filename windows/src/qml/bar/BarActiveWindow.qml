import QtQuick
import Caelestia.Platform 1.0
import ".."
import "../components"

// qmllint disable unqualified

Item {
    id: root

    required property var theme
    required property var panelController
    required property string screenName
    required property real screenY

    readonly property string windowTitle: Platform.activeWindowTitle
    readonly property string windowIcon: Platform.activeWindowIcon
    readonly property int maxTitleWidth: 220

    implicitWidth: theme.barInnerWidth
    implicitHeight: iconItem.height + title.implicitWidth + title.anchors.topMargin
    visible: windowTitle.length > 0 || windowIcon.length > 0

    Item {
        id: iconItem

        anchors.horizontalCenter: parent.horizontalCenter
        width: theme.iconSmall
        height: theme.iconSmall

        Image {
            id: iconImage

            anchors.fill: parent
            source: root.windowIcon
            sourceSize.width: 32
            sourceSize.height: 32
            fillMode: Image.PreserveAspectFit
            smooth: true
            visible: source !== ""
        }
    }

    TextMetrics {
        id: metrics

        text: root.windowTitle
        font: title.font
        elide: Text.ElideRight
        elideWidth: Math.max(40, root.maxTitleWidth - iconItem.height)
    }

    StyledText {
        id: title

        anchors.horizontalCenter: iconItem.horizontalCenter
        anchors.top: iconItem.bottom
        anchors.topMargin: root.theme.spacingSmall
        theme: root.theme
        role: "bodySmall"
        text: metrics.elidedText
        color: root.theme.primary
        animate: true
        width: implicitHeight
        height: implicitWidth

        transform: [
            Rotation {
                angle: 90
                origin.x: title.implicitHeight / 2
                origin.y: title.implicitHeight / 2
            }
        ]
    }

    // Windows has no "active window" popout out of the box; the title rail is
    // the natural door into the port of the Linux windowinfo module.
    StateLayer {
        anchors.fill: parent
        theme: root.theme
        radius: width / 2
        disabled: root.windowTitle.length === 0
        onClicked: {
            const centre = root.mapToItem(null, 0, root.height / 2);
            root.panelController.togglePanel("window", root.screenName,
                root.screenY + centre.y);
        }
    }
}
