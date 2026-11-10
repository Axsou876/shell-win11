import QtQuick
import ".."

// Content viewport of a bar popout. The blob surface is drawn by
// ScreenSurface; this item only clips and reveals the content, exactly like
// the original ClipWrapper.
Item {
    id: root

    required property var theme

    property bool opened: false
    property real contentWidth: 320
    property real contentHeight: 0
    property real deformScaleX: 1
    property real deformScaleY: 1
    property real deformOriginX: width / 2

    transform: Scale {
        xScale: root.deformScaleX
        yScale: root.deformScaleY
        origin.x: root.deformOriginX
        origin.y: root.height / 2
    }

    readonly property real padding: theme.paddingLarge
    readonly property real fullWidth: contentWidth + padding * 2
    readonly property real targetHeight: contentHeight + padding * 2
    readonly property Item contentParent: padded

    width: opened ? fullWidth : 0
    height: targetHeight

    Behavior on width {
        Anim {
            type: Anim.DefaultSpatial
        }
    }

    opacity: opened ? 1 : 0

    Behavior on opacity {
        Anim {
            type: Anim.DefaultEffects
        }
    }

    // Clicks on empty panel space must not fall through to the backdrop.
    // The hover halo lives in OverlayWindow and is driven by raw mouse input,
    // so it also covers child buttons without stealing their clicks.
    MouseArea {
        anchors.fill: parent
        onClicked: {}
    }

    Item {
        id: clipItem

        width: root.width
        height: root.height
        clip: true

        Item {
            width: root.fullWidth
            height: root.targetHeight

            Item {
                id: padded

                anchors.fill: parent
                anchors.margins: root.padding
            }
        }
    }
}
