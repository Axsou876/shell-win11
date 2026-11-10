import QtQuick
import Caelestia.Platform 1.0
import ".."
import "../components"

Item {
    id: root

    required property var theme

    Row {
        anchors.fill: parent
        spacing: root.theme.spacingExtraLarge
        visible: Media.available

        RoundedClip {
            width: Math.min(parent.height, 260)
            height: width
            radius: root.theme.roundingExtraLarge

            Rectangle {
                anchors.fill: parent
                color: root.theme.surfaceContainerHigh

                Image {
                    anchors.fill: parent
                    source: Media.artworkPath
                        ? "file:///" + Media.artworkPath.replace(/\\/g, "/") : ""
                    visible: Media.artworkPath !== ""
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                }

                MaterialIcon {
                    anchors.centerIn: parent
                    visible: !Media.artworkPath
                    theme: root.theme
                    text: "album"
                    iconSize: 72
                    color: root.theme.onSurfaceVariantColour
                    opacity: 0.7
                }
            }
        }

        Column {
            width: parent.width - 260 - root.theme.spacingExtraLarge
            anchors.verticalCenter: parent.verticalCenter
            spacing: root.theme.spacingSmall

            StyledText {
                width: parent.width
                theme: root.theme
                role: "bodyLarge"
                pointSize: 30
                weight: Font.DemiBold
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
                text: Media.title
            }

            StyledText {
                width: parent.width
                theme: root.theme
                role: "bodyLarge"
                color: root.theme.primary
                elide: Text.ElideRight
                text: Media.artist
            }

            StyledText {
                width: parent.width
                theme: root.theme
                role: "bodyMedium"
                color: root.theme.onSurfaceVariantColour
                elide: Text.ElideRight
                text: Media.album
            }

            StyledText {
                width: parent.width
                theme: root.theme
                role: "labelSmall"
                color: root.theme.onSurfaceVariantColour
                opacity: 0.65
                elide: Text.ElideRight
                text: Media.sourceApp
            }

            Item { width: 1; height: 8 }

            Row {
                spacing: root.theme.spacingMedium

                M3Button {
                    theme: root.theme
                    icon: "skip_previous"
                    variant: "tonal"
                    implicitWidth: 52
                    implicitHeight: 46
                    enabled: Media.canPrevious
                    onClicked: Media.previous()
                }

                M3Button {
                    theme: root.theme
                    icon: Media.playing ? "pause" : "play_arrow"
                    variant: "filled"
                    implicitWidth: 62
                    implicitHeight: 46
                    enabled: Media.canPlayPause
                    onClicked: Media.playPause()
                }

                M3Button {
                    theme: root.theme
                    icon: "skip_next"
                    variant: "tonal"
                    implicitWidth: 52
                    implicitHeight: 46
                    enabled: Media.canNext
                    onClicked: Media.next()
                }
            }
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: root.theme.spacingMedium
        visible: !Media.available

        MaterialIcon {
            anchors.horizontalCenter: parent.horizontalCenter
            theme: root.theme
            text: "music_off"
            iconSize: 54
            color: root.theme.onSurfaceVariantColour
            opacity: 0.75
        }

        StyledText {
            anchors.horizontalCenter: parent.horizontalCenter
            theme: root.theme
            role: "bodyLarge"
            weight: Font.DemiBold
            color: root.theme.onSurfaceVariantColour
            text: "Aucune session média active"
        }

        StyledText {
            anchors.horizontalCenter: parent.horizontalCenter
            theme: root.theme
            role: "bodySmall"
            color: root.theme.onSurfaceVariantColour
            opacity: 0.7
            text: "Ouvrez un lecteur compatible avec les contrôles média Windows."
        }
    }
}
