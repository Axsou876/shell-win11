pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Caelestia.Platform 1.0
import "../components"

// Now-playing card driven by the Windows GSMTC session. Playback buttons are
// only enabled when the source app reports the capability, so a dead button
// never looks like a broken shell.
Rectangle {
    id: root

    required property var theme
    property real scaleFactor: 1

    readonly property bool playing: Media.available && Media.title !== ""
    readonly property string artwork: Media.artworkPath !== ""
        ? "file:///" + Media.artworkPath.replace(/\\/g, "/") : ""

    color: root.theme.surfaceContainer
    radius: root.theme.roundingExtraLarge
    clip: true

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.theme.paddingLarge
        spacing: root.theme.spacingSmall
        visible: root.playing

        Item {
            Layout.fillHeight: true
        }

        RoundedClip {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Math.round(132 * root.scaleFactor)
            Layout.preferredHeight: Math.round(132 * root.scaleFactor)
            radius: root.theme.roundingLarge
            color: root.theme.surfaceContainerHighest

            Image {
                anchors.fill: parent
                source: root.artwork
                visible: root.artwork !== ""
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
            }

            MaterialIcon {
                anchors.centerIn: parent
                visible: root.artwork === ""
                theme: root.theme
                text: "music_note"
                iconSize: Math.round(48 * root.scaleFactor)
                color: root.theme.onSurfaceVariantColour
            }
        }

        StyledText {
            Layout.fillWidth: true
            Layout.topMargin: root.theme.spacingSmall
            theme: root.theme
            role: "bodyMedium"
            weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            text: Media.title
        }

        StyledText {
            Layout.fillWidth: true
            theme: root.theme
            role: "labelSmall"
            color: root.theme.onSurfaceVariantColour
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            text: Media.artist
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: root.theme.spacingSmall
            spacing: root.theme.spacingMedium

            M3Button {
                theme: root.theme
                icon: "skip_previous"
                variant: "text"
                implicitWidth: Math.round(38 * root.scaleFactor)
                implicitHeight: implicitWidth
                enabled: Media.canPrevious
                onClicked: Media.previous()
            }

            M3Button {
                theme: root.theme
                icon: Media.playing ? "pause" : "play_arrow"
                variant: "filled"
                implicitWidth: Math.round(48 * root.scaleFactor)
                implicitHeight: implicitWidth
                enabled: Media.canPlayPause
                onClicked: Media.playPause()
            }

            M3Button {
                theme: root.theme
                icon: "skip_next"
                variant: "text"
                implicitWidth: Math.round(38 * root.scaleFactor)
                implicitHeight: implicitWidth
                enabled: Media.canNext
                onClicked: Media.next()
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }

    // The card keeps its space when nothing plays; the empty state explains
    // why instead of showing a blank rectangle.
    Column {
        anchors.centerIn: parent
        width: parent.width - root.theme.paddingExtraLarge
        spacing: root.theme.spacingSmall
        visible: !root.playing

        MaterialIcon {
            anchors.horizontalCenter: parent.horizontalCenter
            theme: root.theme
            text: "music_off"
            iconSize: Math.round(38 * root.scaleFactor)
            color: root.theme.onSurfaceVariantColour
        }

        StyledText {
            width: parent.width
            theme: root.theme
            role: "labelSmall"
            color: root.theme.onSurfaceVariantColour
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: "Aucun lecteur actif"
        }
    }
}
