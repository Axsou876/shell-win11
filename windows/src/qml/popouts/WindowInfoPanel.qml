import QtQuick
import QtQuick.Layouts
import Caelestia.Platform 1.0
import ".."
import "../components"

// qmllint disable unqualified

// Windows counterpart of modules/windowinfo. There is no compositor protocol
// to ask for the focused client, so the details come from Win32 and the
// preview from a PrintWindow capture served by the "windowpreview" provider.
Item {
    id: root

    required property var theme
    required property var controller

    readonly property var info: AppWindows.focused
    readonly property bool hasWindow: AppWindows.focused.valid === true
    // The WM owns geometry: a tiled window is re-laid-out on every rescan, so
    // Float/Tile is the action that actually sticks. The published list keeps
    // the button label honest after a layout change.
    readonly property bool isFloating: root.hasWindow
        && WindowManager.floating.indexOf(root.info.id) !== -1

    readonly property string stateText: {
        if (!root.hasWindow)
            return "";
        const states = [];
        if (root.info.maximized)
            states.push("Agrandie");
        if (root.info.minimized)
            states.push("Réduite");
        if (root.info.fullscreen)
            states.push("Plein écran");
        if (root.info.topmost)
            states.push("Toujours au-dessus");
        if (root.isFloating)
            states.push("Flottante");
        return states.length > 0 ? states.join(" · ") : "Normale";
    }

    implicitWidth: theme.popoutWindowWidth
    implicitHeight: content.implicitHeight

    // Tracking drives a fast poll and a frame capture; both are only worth
    // their cost while this panel exists.
    Component.onCompleted: AppWindows.trackFocused(true, Platform.activeWindowId)
    Component.onDestruction: AppWindows.trackFocused(false)

    Column {
        id: content

        width: parent.width
        spacing: root.theme.spacingMedium

        PanelHeader {
            width: parent.width
            theme: root.theme
            title: "Fenêtre active"
            subtitle: root.hasWindow ? root.info.app : "Aucune fenêtre"
            onClose: root.controller.closePanel()
        }

        Rectangle {
            width: parent.width
            height: 1
            color: root.theme.outlineVariant
            opacity: 0.6
        }

        Rectangle {
            width: parent.width
            height: 170
            radius: root.theme.roundingMedium
            color: root.theme.surfaceContainer
            clip: true

            Image {
                id: preview

                anchors.fill: parent
                anchors.margins: 2
                source: root.hasWindow && AppWindows.previewRevision > 0
                    ? "image://windowpreview/" + AppWindows.previewRevision : ""
                fillMode: Image.PreserveAspectFit
                smooth: true
                // Every capture is a new source string; caching them would
                // grow QQuickPixmapCache by a frame every 700 ms.
                cache: false
            }

            MaterialIcon {
                anchors.centerIn: parent
                visible: preview.status !== Image.Ready
                theme: root.theme
                text: root.hasWindow ? "photo_size_select_actual" : "tab"
                iconSize: 42
                color: root.theme.onSurfaceVariantColour
            }
        }

        Column {
            width: parent.width
            spacing: 2

            StyledText {
                width: parent.width
                theme: root.theme
                role: "bodyLarge"
                weight: Font.DemiBold
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
                text: root.hasWindow ? root.info.title : "Aucune fenêtre active"
            }

            StyledText {
                width: parent.width
                visible: root.hasWindow
                theme: root.theme
                role: "labelSmall"
                color: root.theme.primary
                elide: Text.ElideRight
                text: root.hasWindow ? root.info.className : ""
            }
        }

        Column {
            width: parent.width
            spacing: root.theme.spacingExtraSmall
            visible: root.hasWindow

            Detail {
                theme: root.theme
                icon: "tag"
                label: "Adresse"
                value: root.hasWindow ? root.info.address : ""
            }

            Detail {
                theme: root.theme
                icon: "open_with"
                label: "Position"
                value: root.hasWindow ? root.info.x + ", " + root.info.y : ""
            }

            Detail {
                theme: root.theme
                icon: "aspect_ratio"
                label: "Taille"
                value: root.hasWindow ? root.info.width + " × " + root.info.height : ""
            }

            Detail {
                theme: root.theme
                icon: "monitor"
                label: "Écran"
                value: root.hasWindow ? root.info.monitor : ""
            }

            Detail {
                theme: root.theme
                icon: "memory"
                label: "Processus"
                value: root.hasWindow
                    ? root.info.app + " (" + root.info.pid + ")" : ""
            }

            Detail {
                theme: root.theme
                icon: "straighten"
                label: "DPI"
                value: root.hasWindow
                    ? root.info.dpi + " (" + Math.round(root.info.dpi / 96 * 100) + " %)" : ""
            }

            Detail {
                theme: root.theme
                icon: "layers"
                label: "État"
                value: root.stateText
            }
        }

        Column {
            width: parent.width
            spacing: 2
            visible: root.hasWindow

            StyledText {
                theme: root.theme
                role: "labelSmall"
                color: root.theme.onSurfaceVariantColour
                text: "Chemin"
            }

            StyledText {
                width: parent.width
                theme: root.theme
                role: "labelSmall"
                color: root.theme.onSurfaceVariantColour
                wrapMode: Text.WrapAnywhere
                maximumLineCount: 2
                elide: Text.ElideRight
                text: root.hasWindow ? root.info.path : ""
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: root.theme.outlineVariant
            opacity: 0.6
        }

        RowLayout {
            width: parent.width
            spacing: root.theme.spacingSmall

            M3Button {
                Layout.fillWidth: true
                theme: root.theme
                text: "Réduire"
                icon: "minimize"
                variant: "tonal"
                enabled: root.hasWindow && !root.info.minimized
                onClicked: AppWindows.minimizeFocused()
            }

            M3Button {
                Layout.fillWidth: true
                theme: root.theme
                text: root.isFloating ? "Ancrer" : "Flotter"
                icon: root.isFloating ? "grid_view" : "picture_in_picture_alt"
                variant: "tonal"
                enabled: root.hasWindow
                onClicked: WindowManager.setFloating(root.info.id, !root.isFloating)
            }
        }

        M3Button {
            width: parent.width
            theme: root.theme
            text: "Fermer la fenêtre"
            icon: "close"
            variant: "text"
            foreground: root.theme.error
            enabled: root.hasWindow
            onClicked: AppWindows.closeFocused()
        }

        StyledText {
            width: parent.width
            visible: AppWindows.actionError !== ""
            theme: root.theme
            role: "labelSmall"
            color: root.theme.error
            wrapMode: Text.Wrap
            text: AppWindows.actionError
        }
    }

    // Local icon + label + value row; the Linux panel's Detail component has
    // no other consumer and the layout here is a single line per field.
    component Detail: RowLayout {
        id: detail

        property var theme
        property string icon: ""
        property string label: ""
        property string value: ""

        width: parent ? parent.width : 0
        spacing: theme.spacingSmall

        MaterialIcon {
            Layout.alignment: Qt.AlignVCenter
            theme: detail.theme
            text: detail.icon
            iconSize: 16
            color: detail.theme.onSurfaceVariantColour
        }

        StyledText {
            Layout.alignment: Qt.AlignVCenter
            theme: detail.theme
            role: "labelSmall"
            color: detail.theme.onSurfaceVariantColour
            text: detail.label
        }

        Item {
            Layout.fillWidth: true
        }

        StyledText {
            Layout.alignment: Qt.AlignVCenter
            Layout.maximumWidth: detail.width * 0.62
            theme: detail.theme
            role: "labelSmall"
            horizontalAlignment: Text.AlignRight
            elide: Text.ElideMiddle
            text: detail.value
        }
    }
}
