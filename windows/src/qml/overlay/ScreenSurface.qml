pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Effects
import ".."

// Full-screen Caelestia surface: a 10 px frame around a rounded wallpaper
// hole, with the 60 px bar on the left and the large panels merged into the
// frame by a circular SDF union. This is the Windows equivalent of the
// original BlobGroup/BlobInvertedRect composition.
Item {
    id: surface

    required property var theme
    required property var controller
    required property string screenName

    property rect popoutRect: Qt.rect(0, 0, 0, 0)
    property bool popoutVisible: false

    // The original popout blob extends 20% into the bar to keep the junction
    // stable while the surface deforms vertically. The content stays clipped
    // to popoutRect; only the surface grows to the left.
    readonly property real popoutExtra: popoutRect.width * 0.2
    readonly property real popoutBlobX: popoutRect.x - popoutExtra
    readonly property real popoutBlobWidth: popoutRect.width + popoutExtra
    readonly property real popoutDeformOriginX: popoutRect.width / 2 - popoutExtra / 2

    readonly property int border: theme.borderThickness
    readonly property int barWidth: theme.barRailWidth
    readonly property int sidebarW: theme.sidebarWidth
    readonly property int dashboardW: theme.dashboardWidth

    readonly property bool sidebarActive: (controller.currentPanel === "sidebar"
        || controller.currentPanel === "notifications")
        && controller.currentScreenName === screenName && !controller.closing
    readonly property bool dashboardActive: controller.currentPanel === "dashboard"
        && controller.currentScreenName === screenName && !controller.closing
    readonly property bool launcherActive: controller.currentPanel === "launcher"
        && controller.currentScreenName === screenName && !controller.closing

    property real sidebarOffset: sidebarActive ? 0 : 1
    property real dashboardOffset: dashboardActive ? 0 : 1
    property real launcherOffset: launcherActive ? 0 : 1

    Behavior on sidebarOffset {
        Anim {
            type: Anim.DefaultSpatial
        }
    }

    Behavior on dashboardOffset {
        Anim {
            type: Anim.DefaultSpatial
        }
    }

    Behavior on launcherOffset {
        Anim {
            type: Anim.DefaultSpatial
        }
    }

    readonly property real sidebarX: width - border - sidebarW + (sidebarW + 5) * sidebarOffset
    readonly property real sidebarY: border + theme.paddingLarge
    readonly property real sidebarHeight: height - border * 2 - theme.paddingLarge * 2
    readonly property real sidebarOpacity: 1 - sidebarOffset
    readonly property bool sidebarVisible: sidebarOffset < 0.999

    readonly property real dashboardX: (width - dashboardW) / 2
    readonly property real dashboardY: border - (theme.dashboardHeight + 5) * dashboardOffset
    readonly property real dashboardOpacity: 1 - dashboardOffset
    readonly property bool dashboardVisible: dashboardOffset < 0.999

    readonly property real launcherX: (width - theme.launcherWidth) / 2
    readonly property real launcherY: height - border - theme.launcherHeight - theme.launcherBottomMargin
        + (theme.launcherHeight + theme.launcherBottomMargin + border) * launcherOffset
    readonly property real launcherOpacity: 1 - launcherOffset
    readonly property bool launcherVisible: launcherOffset < 0.999

    readonly property color surfaceColour: theme.surface

    SpringDeform {
        id: sidebarSpring

        enabled: surface.sidebarVisible
        deformScale: 0.03 * surface.theme.appearanceDeformScale / 10000
        centre: Qt.point(surface.sidebarX + surface.sidebarW / 2,
            surface.sidebarY + surface.sidebarHeight / 2)
    }

    SpringDeform {
        id: dashboardSpring

        enabled: surface.dashboardVisible
        deformScale: 0.10 * surface.theme.appearanceDeformScale / 10000
        centre: Qt.point(surface.dashboardX + surface.dashboardW / 2,
            surface.dashboardY + surface.theme.dashboardHeight / 2)
    }

    SpringDeform {
        id: popoutSpring

        enabled: surface.popoutVisible
        deformScale: 0.15 * surface.theme.appearanceDeformScale / 10000
        centre: Qt.point(surface.popoutBlobX + surface.popoutBlobWidth / 2,
            surface.popoutRect.y + surface.popoutRect.height / 2)
    }

    SpringDeform {
        id: launcherSpring

        enabled: surface.launcherVisible
        deformScale: 0.15 * surface.theme.appearanceDeformScale / 10000
        centre: Qt.point(surface.launcherX + surface.theme.launcherWidth / 2,
            surface.launcherY + surface.theme.launcherHeight / 2)
    }

    // The spring is axis-aligned because the sidebar, dashboard and popout
    // motions are horizontal or vertical. The original also supports a full
    // symmetric 2x2 matrix for diagonal motion (detached popouts); that case
    // is not used in step 1c.
    readonly property real sidebarScaleX: sidebarSpring.scaleX
    readonly property real sidebarScaleY: sidebarSpring.scaleY
    readonly property real dashboardScaleX: dashboardSpring.scaleX
    readonly property real dashboardScaleY: dashboardSpring.scaleY
    readonly property real popoutScaleX: popoutSpring.scaleX
    readonly property real popoutScaleY: popoutSpring.scaleY
    readonly property real launcherScaleX: launcherSpring.scaleX
    readonly property real launcherScaleY: launcherSpring.scaleY

    readonly property vector4d sidebarInverse: sidebarSpring.inverse
    readonly property vector4d dashboardInverse: dashboardSpring.inverse
    readonly property vector4d popoutInverse: popoutSpring.inverse
    readonly property vector4d launcherInverse: launcherSpring.inverse

    Item {
        anchors.fill: parent
        layer.enabled: true
        // The layer must stay live. It is not just a shadow: the blob shader
        // below draws the bar frame *and* every panel surface (sidebar,
        // dashboard, popout, launcher) from uniforms that change as panels
        // open. With live: false the layer kept a stale texture and the
        // popouts rendered with no background at all.
        layer.live: true
        layer.smooth: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            blurMax: 15
            shadowColor: Qt.alpha(surface.theme.shadow, surface.theme.shadowOpacity)
        }

        ShaderEffect {
            anchors.fill: parent

            // Keep this property order in sync with blob.vert/frag.
            property vector4d sizeParams: Qt.vector4d(surface.width, surface.height,
                surface.theme.borderRounding, surface.theme.borderSmoothing)
            property vector4d frame: Qt.vector4d(surface.barWidth, surface.border,
                Math.max(0, surface.width - surface.barWidth - surface.border),
                Math.max(0, surface.height - surface.border * 2))
            property color colour: surface.surfaceColour
            property vector4d panel0: surface.sidebarVisible
                ? Qt.vector4d(surface.sidebarX, surface.sidebarY, surface.sidebarW, surface.sidebarHeight)
                : Qt.vector4d(0, 0, 0, 0)
            property vector4d radii0: Qt.vector4d(surface.theme.panelRadius, surface.theme.panelRadius,
                surface.theme.panelRadius, surface.theme.panelRadius)
            property vector4d deform0: surface.sidebarInverse
            property vector4d panel1: surface.dashboardVisible
                ? Qt.vector4d(surface.dashboardX, surface.dashboardY,
                    surface.dashboardW, surface.theme.dashboardHeight)
                : Qt.vector4d(0, 0, 0, 0)
            property vector4d radii1: Qt.vector4d(surface.theme.panelRadius, surface.theme.panelRadius,
                surface.theme.panelRadius, surface.theme.panelRadius)
            property vector4d deform1: surface.dashboardInverse
            property vector4d panel2: surface.popoutVisible
                ? Qt.vector4d(surface.popoutBlobX, surface.popoutRect.y,
                    surface.popoutBlobWidth, surface.popoutRect.height)
                : Qt.vector4d(0, 0, 0, 0)
            property vector4d radii2: Qt.vector4d(surface.theme.panelRadius, surface.theme.panelRadius,
                surface.theme.panelRadius, surface.theme.panelRadius)
            property vector4d deform2: surface.popoutInverse
            property vector4d panel3: surface.launcherVisible
                ? Qt.vector4d(surface.launcherX, surface.launcherY,
                    surface.theme.launcherWidth, surface.theme.launcherHeight)
                : Qt.vector4d(0, 0, 0, 0)
            property vector4d radii3: Qt.vector4d(surface.theme.panelRadius, surface.theme.panelRadius,
                surface.theme.panelRadius, surface.theme.panelRadius)
            property vector4d deform3: surface.launcherInverse

            vertexShader: "qrc:/src/shaders/blob.vert.qsb"
            fragmentShader: "qrc:/src/shaders/blob.frag.qsb"

            Behavior on colour {
                CAnim {}
            }
        }
    }

}






