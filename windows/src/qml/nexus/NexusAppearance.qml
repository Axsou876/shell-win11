import QtQuick
import QtQuick.Layouts
import Caelestia.Platform 1.0
import ".."
import "../components"

// Appearance page: wallpapers found by the wallpaper service, plus the theme
// handover. Every thumbnail is loaded lazily and cropped, because the scan
// returns arbitrary aspect ratios and cannot pre-scale anything.
Flickable {
    id: root

    required property var theme
    required property var controller

    contentWidth: width
    contentHeight: content.implicitHeight
    clip: true
    boundsBehavior: Flickable.StopAtBounds

    function fileUrl(path) {
        return "file:///" + String(path).replace(/\\/g, "/");
    }

    ColumnLayout {
        id: content

        width: root.width
        spacing: root.theme.spacingLarge

        StyledText {
            theme: root.theme
            role: "titleSmall"
            text: "Fond d'écran"
        }

        StyledText {
            Layout.fillWidth: true
            theme: root.theme
            role: "labelSmall"
            color: root.theme.onSurfaceVariantColour
            wrapMode: Text.WordWrap
            text: Wallpaper.wallpapers.length === 0
                ? "Aucune image trouvée dans les dossiers Windows."
                : "Cliquez sur une vignette : Windows applique l'image immédiatement."
        }

        GridView {
            id: grid

            Layout.fillWidth: true
            // Exactly two rows: a third row sliced in half reads as a bug, and
            // the grid scrolls for the rest.
            Layout.preferredHeight: 172
            cellWidth: 132
            cellHeight: 86
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: Wallpaper.wallpapers

            delegate: Item {
                id: cell

                required property var modelData

                width: grid.cellWidth
                height: grid.cellHeight

                readonly property bool current: modelData.path === Wallpaper.currentPath

                Rectangle {
                    id: frame

                    anchors.fill: parent
                    anchors.margins: 3
                    radius: root.theme.roundingMedium
                    color: root.theme.surfaceContainerHigh
                    border.width: cell.current ? 2 : 1
                    border.color: cell.current ? root.theme.primary : root.theme.outlineVariant
                    clip: true

                    Image {
                        anchors.fill: parent
                        anchors.margins: 1
                        source: root.fileUrl(cell.modelData.path)
                        sourceSize: Qt.size(Math.round(frame.width * 2), Math.round(frame.height * 2))
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        cache: true
                        smooth: true
                    }

                    StyledText {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.margins: 6
                        theme: root.theme
                        role: "labelSmall"
                        color: "#ffffff"
                        elide: Text.ElideRight
                        style: Text.Outline
                        styleColor: "#88000000"
                        text: cell.modelData.name
                    }
                }

                StateLayer {
                    anchors.fill: frame
                    theme: root.theme
                    radius: frame.radius
                    onClicked: Wallpaper.apply(cell.modelData.path)
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: 4
            height: 1
            color: root.theme.outlineVariant
            opacity: 0.6
        }

        StyledText {
            theme: root.theme
            role: "titleSmall"
            text: "Thème"
        }

        M3ListItem {
            Layout.fillWidth: true
            theme: root.theme
            icon: Platform.darkMode ? "dark_mode" : "light_mode"
            title: Platform.darkMode ? "Mode sombre" : "Mode clair"
            subtitle: "Caelestia suit le thème des applications Windows"
            trailingText: "F8"
            onClicked: Qt.openUrlExternally("ms-settings:personalization")
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            M3Button {
                theme: root.theme
                text: "Couleurs et effets"
                icon: "palette"
                variant: "tonal"
                onClicked: Qt.openUrlExternally("ms-settings:personalization-colors")
            }

            M3Button {
                theme: root.theme
                text: "Arrière-plan Windows"
                icon: "wallpaper"
                variant: "text"
                onClicked: Qt.openUrlExternally("ms-settings:personalization-background")
            }

            Item {
                Layout.fillWidth: true
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
