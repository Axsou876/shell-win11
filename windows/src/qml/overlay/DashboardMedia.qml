import QtQuick
import Caelestia.Platform 1.0
import ".."
import "../components"

// Media tab reconstructed from Media.qml + media/*: CoverVisualiser on the
// left, real GSMTC details in the centre and an honest lyrics panel on the
// right (GSMTC does not expose lyrics).
Item {
    id: root

    required property var theme

    readonly property bool available: Media.available
    readonly property real progress: Media.durationMs > 0
        ? Math.max(0, Math.min(1, Media.positionMs / Media.durationMs)) : 0

    function timeText(ms) {
        if (ms <= 0)
            return "0:00";
        const total = Math.floor(ms / 1000);
        const minutes = Math.floor(total / 60);
        const seconds = total % 60;
        return minutes + ":" + (seconds < 10 ? "0" : "") + seconds;
    }

    // Decorative background shapes from the Linux media tab, kept monochrome.
    Rectangle {
        x: root.width * 0.08
        y: -root.height * 0.2
        width: root.height * 0.8
        height: width
        radius: width / 2
        color: Qt.alpha(root.theme.surfaceContainerHigh, 0.35)
    }

    Rectangle {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: -root.height * 0.15
        anchors.bottomMargin: -root.height * 0.3
        width: root.height
        height: width
        radius: width / 2
        color: Qt.alpha(root.theme.surfaceContainer, 0.5)
    }

    Row {
        anchors.fill: parent
        spacing: root.theme.spacingExtraLarge
        visible: root.available

        // --- Cover visualiser ---------------------------------------------
        Item {
            id: coverSection

            width: 300
            height: parent.height

            RoundedClip {
                id: cover

                anchors.centerIn: parent
                width: 200
                height: 200
                radius: root.theme.roundingExtraLarge
                color: root.theme.surfaceContainerHighest

                Image {
                    anchors.fill: parent
                    source: Media.artworkPath !== ""
                        ? "file:///" + Media.artworkPath.replace(/\\/g, "/") : ""
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                }

                MaterialIcon {
                    anchors.centerIn: parent
                    visible: Media.artworkPath === ""
                    theme: root.theme
                    text: "album"
                    iconSize: 64
                    color: root.theme.onSurfaceVariantColour
                }
            }

            CircularProgress {
                anchors.centerIn: cover
                width: 236
                height: 236
                theme: root.theme
                value: root.progress
                startAngle: -180
                sweepAngle: 180
                strokeWidth: 6
                fgColour: root.theme.primary
            }

            Column {
                anchors.top: cover.bottom
                anchors.topMargin: root.theme.spacingLarge
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 2

                StyledText {
                    anchors.horizontalCenter: parent.horizontalCenter
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.onSurfaceVariantColour
                    text: "Visualiseur audio temps réel"
                }

                StyledText {
                    anchors.horizontalCenter: parent.horizontalCenter
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.outline
                    text: "indisponible sous Windows"
                }
            }
        }

        // --- Details -------------------------------------------------------
        Item {
            id: details

            width: parent.width - coverSection.width - lyrics.width
                - root.theme.spacingExtraLarge * 2
            height: parent.height

            Column {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width
                spacing: root.theme.spacingExtraSmall

                StyledText {
                    width: parent.width
                    theme: root.theme
                    role: "bodyLarge"
                    pointSize: 24
                    weight: Font.DemiBold
                    elide: Text.ElideRight
                    maximumLineCount: 2
                    wrapMode: Text.WordWrap
                    text: Media.title !== "" ? Media.title : "Titre inconnu"
                }

                StyledText {
                    width: parent.width
                    theme: root.theme
                    role: "titleSmall"
                    color: root.theme.onSurfaceVariantColour
                    elide: Text.ElideRight
                    text: Media.artist !== "" ? Media.artist : "Artiste inconnu"
                }

                StyledText {
                    width: parent.width
                    theme: root.theme
                    role: "titleSmall"
                    color: root.theme.secondary
                    elide: Text.ElideRight
                    text: Media.album !== "" ? Media.album : "Album inconnu"
                }

                Item { width: 1; height: 12 }

                Row {
                    width: parent.width
                    spacing: root.theme.spacingSmall

                    StyledText {
                        anchors.verticalCenter: parent.verticalCenter
                        theme: root.theme
                        role: "labelSmall"
                        color: root.theme.onSurfaceVariantColour
                        text: root.timeText(Media.positionMs)
                        width: 42
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Meter {
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - 100
                        theme: root.theme
                        value: root.progress * 100
                    }

                    StyledText {
                        anchors.verticalCenter: parent.verticalCenter
                        theme: root.theme
                        role: "labelSmall"
                        color: root.theme.onSurfaceVariantColour
                        text: root.timeText(Media.durationMs)
                        width: 42
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                Item { width: 1; height: 10 }

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: root.theme.spacingMedium

                    M3Button {
                        theme: root.theme
                        icon: "skip_previous"
                        variant: "tonal"
                        implicitWidth: 48
                        implicitHeight: 44
                        enabled: Media.canPrevious
                        onClicked: Media.previous()
                    }

                    M3Button {
                        theme: root.theme
                        icon: Media.playing ? "pause" : "play_arrow"
                        variant: "filled"
                        implicitWidth: 60
                        implicitHeight: 48
                        enabled: Media.canPlayPause
                        onClicked: Media.playPause()
                    }

                    M3Button {
                        theme: root.theme
                        icon: "skip_next"
                        variant: "tonal"
                        implicitWidth: 48
                        implicitHeight: 44
                        enabled: Media.canNext
                        onClicked: Media.next()
                    }
                }

                Item { width: 1; height: 8 }

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: sourceRow.implicitWidth + root.theme.paddingMedium * 2
                    height: 24
                    radius: 12
                    color: root.theme.secondaryContainer

                    Row {
                        id: sourceRow

                        anchors.centerIn: parent
                        spacing: 4

                        MaterialIcon {
                            anchors.verticalCenter: parent.verticalCenter
                            theme: root.theme
                            text: "graphic_eq"
                            iconSize: 14
                            color: root.theme.onSecondaryContainerColour
                        }

                        StyledText {
                            anchors.verticalCenter: parent.verticalCenter
                            theme: root.theme
                            role: "labelSmall"
                            color: root.theme.onSecondaryContainerColour
                            text: Media.sourceApp !== ""
                                ? Media.sourceApp.split("!").pop().replace(".exe", "")
                                : "GSMTC"
                        }
                    }
                }
            }
        }

        // --- Lyrics --------------------------------------------------------
        Rectangle {
            id: lyrics

            width: 300
            height: parent.height
            radius: root.theme.roundingLarge
            color: root.theme.surfaceContainer
            border.width: 1
            border.color: Qt.alpha(root.theme.outlineVariant, 0.25)

            Column {
                anchors.centerIn: parent
                width: parent.width - root.theme.paddingLarge * 2
                spacing: root.theme.spacingSmall

                MaterialIcon {
                    anchors.horizontalCenter: parent.horizontalCenter
                    theme: root.theme
                    text: "lyrics"
                    iconSize: 42
                    color: root.theme.onSurfaceVariantColour
                    opacity: 0.75
                }

                StyledText {
                    anchors.horizontalCenter: parent.horizontalCenter
                    theme: root.theme
                    role: "bodyMedium"
                    weight: Font.DemiBold
                    text: "Paroles indisponibles"
                }

                StyledText {
                    width: parent.width
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.onSurfaceVariantColour
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    text: "GSMTC ne fournit pas les paroles des sessions média Windows."
                }
            }
        }
    }

    // --- Empty state -------------------------------------------------------
    Column {
        anchors.centerIn: parent
        spacing: root.theme.spacingMedium
        visible: !root.available

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 104
            height: 104
            radius: 52
            color: root.theme.primaryContainer

            MaterialIcon {
                anchors.centerIn: parent
                theme: root.theme
                text: "queue_music"
                iconSize: 52
                color: root.theme.onPrimaryContainerColour
            }
        }

        StyledText {
            anchors.horizontalCenter: parent.horizontalCenter
            theme: root.theme
            role: "bodyLarge"
            pointSize: 22
            weight: Font.DemiBold
            text: "Aucune lecture"
        }

        StyledText {
            anchors.horizontalCenter: parent.horizontalCenter
            theme: root.theme
            role: "bodyMedium"
            color: root.theme.onSurfaceVariantColour
            text: "Lancez un média compatible GSMTC pour l’afficher ici."
        }
    }
}
