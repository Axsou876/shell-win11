pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Caelestia.Platform 1.0
import ".."
import "../components"

// Dashboard home rebuilt from the Linux Dash.qml grid. Layout uses the same
// GridLayout/span structure as the original so every card shares one visual
// grid instead of hard-coded positions.
GridLayout {
    id: root

    required property var theme

    // False while the dashboard is closed: the page stays loaded, but nothing
    // here may keep repainting the fullscreen overlay while it is off-screen
    // (see CircularProgress.animate).
    property bool animations: true

    signal openWeather()

    columns: 6
    rowSpacing: theme.spacingMedium
    columnSpacing: theme.spacingMedium

    Timer {
        id: clockTimer

        interval: 1000
        running: root.animations
        repeat: true
        triggeredOnStart: true
        onTriggered: root.now = new Date()
    }

    property date now: new Date()

    function uptimeText() {
        const s = Math.max(0, System.uptimeSeconds);
        const d = Math.floor(s / 86400);
        const h = Math.floor((s % 86400) / 3600);
        const m = Math.floor((s % 3600) / 60);
        let out = "";
        if (d > 0)
            out += d + "j ";
        if (h > 0 || d > 0)
            out += h + "h ";
        return "up " + out + m + "min";
    }

    component Tile: Rectangle {
        id: tile

        required property var theme

        color: theme.surfaceContainer
        border.width: 1
        border.color: Qt.alpha(theme.outlineVariant, 0.25)
    }

    // --- Weather -----------------------------------------------------------
    Tile {
        id: weatherCard

        Layout.row: 0
        Layout.column: 0
        Layout.columnSpan: 2
        Layout.preferredWidth: 275
        Layout.preferredHeight: 180
        radius: root.theme.roundingExtraLarge * 1.5
        theme: root.theme

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: root.openWeather()
        }

        Row {
            anchors.centerIn: parent
            spacing: root.theme.spacingLargeIncreased

            MaterialIcon {
                anchors.verticalCenter: parent.verticalCenter
                theme: root.theme
                text: Weather.available ? Weather.icon : "cloud_off"
                iconSize: 56
                color: root.theme.secondary
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: root.theme.spacingExtraSmall

                StyledText {
                    anchors.horizontalCenter: parent.horizontalCenter
                    theme: root.theme
                    role: "bodyLarge"
                    pointSize: 26
                    weight: Font.DemiBold
                    color: root.theme.primary
                    text: Weather.available ? Weather.temperature : "Météo"
                }

                StyledText {
                    anchors.horizontalCenter: parent.horizontalCenter
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.onSurfaceVariantColour
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    maximumLineCount: 2
                    width: Math.min(implicitWidth, 155)
                    text: Weather.available ? Weather.description : "Non configurée"
                }

                StyledText {
                    anchors.horizontalCenter: parent.horizontalCenter
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.outline
                    text: Weather.available ? Weather.city : "Ouvrir Weather"
                }
            }
        }
    }

    // --- User --------------------------------------------------------------
    Tile {
        id: userCard

        Layout.row: 0
        Layout.column: 2
        Layout.columnSpan: 3
        Layout.preferredWidth: 340
        Layout.fillHeight: true
        radius: root.theme.roundingExtraLarge
        theme: root.theme

        RoundedClip {
            id: pfp

            x: 16
            y: 16
            width: 148
            height: Math.min(148, userCard.height - 32)
            radius: width / 2
            color: root.theme.surfaceContainerHighest

            Image {
                anchors.fill: parent
                source: User.avatarPath !== ""
                    ? "file:///" + User.avatarPath.replace(/\\/g, "/") : ""
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
            }

            MaterialIcon {
                anchors.centerIn: parent
                visible: User.avatarPath === ""
                theme: root.theme
                text: "person_add"
                iconSize: 56
                fill: 1
                color: root.theme.onSurfaceVariantColour
            }
        }

        Rectangle {
            width: 46
            height: 46
            radius: 23
            x: 8
            y: 8
            color: root.theme.primaryContainer

            Logo {
                anchors.centerIn: parent
                width: 28
                height: 28
                theme: root.theme
            }
        }

        Rectangle {
            id: wmChip

            x: pfp.x + pfp.width + 8
            y: 12
            width: Math.min(150, userCard.width - x - 16)
            height: 28
            radius: 14
            color: root.theme.secondaryContainer

            Row {
                id: wmRow

                anchors.centerIn: parent
                spacing: root.theme.spacingExtraSmall

                MaterialIcon {
                    anchors.verticalCenter: parent.verticalCenter
                    theme: root.theme
                    text: "select_window"
                    iconSize: 15
                    color: root.theme.onSecondaryContainerColour
                }

                StyledText {
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.min(implicitWidth, wmChip.width - 42)
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.onSecondaryContainerColour
                    elide: Text.ElideRight
                    // The chip only has room for the product name; the full
                    // build string ("Windows 11 Version 24H2") is elided, so
                    // strip the build and keep the readable name.
                    text: User.windowsVersion.split(" Version ")[0]
                }
            }
        }

        StyledText {
            anchors.left: wmChip.left
            anchors.top: wmChip.bottom
            anchors.topMargin: 6
            width: userCard.width - wmChip.x - 16
            theme: root.theme
            role: "bodyMedium"
            weight: Font.DemiBold
            color: root.theme.onSurfaceColour
            elide: Text.ElideRight
            text: User.displayName
        }

        Rectangle {
            id: uptimeBadge

            x: pfp.x + pfp.width - 42
            y: pfp.y + pfp.height - 42
            width: 44
            height: 44
            radius: 22
            color: root.theme.tertiaryContainer

            MaterialIcon {
                anchors.centerIn: parent
                theme: root.theme
                text: "clock_arrow_up"
                iconSize: 20
                color: root.theme.onTertiaryContainerColour
            }
        }

        StyledText {
            anchors.left: uptimeBadge.right
            anchors.verticalCenter: uptimeBadge.verticalCenter
            anchors.leftMargin: root.theme.spacingSmall
            width: userCard.width - x - root.theme.paddingLarge
            theme: root.theme
            role: "bodySmall"
            color: root.theme.onSurfaceVariantColour
            elide: Text.ElideRight
            text: root.uptimeText()
        }

        Rectangle {
            width: 10
            height: 10
            radius: 5
            x: uptimeBadge.x + 2
            y: uptimeBadge.y - 10
            color: root.theme.secondaryContainer
        }

        Rectangle {
            width: 15
            height: 15
            radius: 7.5
            x: uptimeBadge.x + 14
            y: uptimeBadge.y - 22
            color: root.theme.secondaryContainer
        }
    }

    // --- Media mini card ---------------------------------------------------
    Tile {
        id: mediaCard

        Layout.row: 0
        Layout.column: 5
        Layout.rowSpan: 2
        Layout.preferredWidth: 200
        Layout.fillHeight: true
        radius: root.theme.roundingExtraLarge * 2
        theme: root.theme

        readonly property bool available: Media.available
        readonly property real progress: Media.durationMs > 0
            ? Math.max(0, Math.min(1, Media.positionMs / Media.durationMs)) : 0

        RoundedClip {
            id: homeCover

            anchors.horizontalCenter: parent.horizontalCenter
            y: 24
            width: 150
            height: 150
            radius: 20
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
                text: "music_note"
                iconSize: 44
                color: root.theme.onSurfaceVariantColour
            }
        }

        CircularProgress {
            anchors.centerIn: homeCover
            width: 170
            height: 170
            theme: root.theme
            animate: root.animations
            value: mediaCard.progress
            startAngle: -180
            sweepAngle: 180
            strokeWidth: 6
            fgColour: root.theme.primary
        }

        StyledText {
            id: homeTitle

            anchors.top: homeCover.bottom
            anchors.topMargin: root.theme.spacingMedium
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width - root.theme.paddingLargeIncreased
            theme: root.theme
            role: "titleSmall"
            color: root.theme.primary
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            text: Media.available ? Media.title : "Aucune lecture"
        }

        StyledText {
            anchors.top: homeTitle.bottom
            anchors.topMargin: root.theme.spacingSmall
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width - root.theme.paddingLargeIncreased
            theme: root.theme
            role: "bodySmall"
            color: root.theme.outline
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            text: Media.available ? Media.album : "GSMTC"
        }

        StyledText {
            id: homeArtist

            anchors.top: homeTitle.bottom
            anchors.topMargin: root.theme.spacingSmall + 16
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width - root.theme.paddingLargeIncreased
            theme: root.theme
            role: "bodySmall"
            color: root.theme.secondary
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            // The empty state below already says there is no media session;
            // a lone dash here only reads as a rendering artifact.
            visible: Media.available
            text: Media.artist
        }

        Row {
            id: homeControls

            anchors.top: homeArtist.bottom
            anchors.topMargin: root.theme.spacingMedium
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: root.theme.spacingExtraSmall

            M3Button {
                theme: root.theme
                icon: "skip_previous"
                variant: "text"
                implicitWidth: 34
                implicitHeight: 32
                enabled: Media.canPrevious
                onClicked: Media.previous()
            }

            M3Button {
                theme: root.theme
                icon: Media.playing ? "pause" : "play_arrow"
                variant: "tonal"
                implicitWidth: 44
                implicitHeight: 36
                enabled: Media.canPlayPause
                onClicked: Media.playPause()
            }

            M3Button {
                theme: root.theme
                icon: "skip_next"
                variant: "text"
                implicitWidth: 34
                implicitHeight: 32
                enabled: Media.canNext
                onClicked: Media.next()
            }
        }

        AnimatedImage {
            anchors.top: homeControls.bottom
            anchors.topMargin: root.theme.spacingSmall
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: 28
            anchors.rightMargin: 28
            anchors.bottomMargin: 16
            visible: Media.available
            // A playing GIF is a 60 fps repaint source; keep it frozen while
            // the dashboard is closed.
            playing: root.animations && Media.playing
            source: "qrc:/assets/bongocat.gif"
            fillMode: Image.PreserveAspectFit
            asynchronous: true
        }

        StyledText {
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 16
            anchors.horizontalCenter: parent.horizontalCenter
            visible: !Media.available
            theme: root.theme
            role: "labelSmall"
            color: root.theme.onSurfaceVariantColour
            text: "Aucune session média"
        }
    }

    // --- Date / time -------------------------------------------------------
    Tile {
        id: dateCard

        Layout.row: 1
        Layout.column: 0
        Layout.preferredWidth: 110
        Layout.fillHeight: true
        radius: root.theme.roundingLarge
        theme: root.theme

        Column {
            anchors.centerIn: parent
            spacing: -4

            StyledText {
                anchors.horizontalCenter: parent.horizontalCenter
                theme: root.theme
                role: "clock"
                pointSize: 28
                weight: Font.DemiBold
                color: root.theme.secondary
                text: Qt.formatDateTime(root.now, "HH")
            }

            StyledText {
                anchors.horizontalCenter: parent.horizontalCenter
                theme: root.theme
                role: "clock"
                pointSize: 25
                color: root.theme.primary
                text: "•••"
            }

            StyledText {
                anchors.horizontalCenter: parent.horizontalCenter
                theme: root.theme
                role: "clock"
                pointSize: 28
                weight: Font.DemiBold
                color: root.theme.secondary
                text: Qt.formatDateTime(root.now, "mm")
            }
        }
    }

    // --- Calendar ----------------------------------------------------------
    Tile {
        id: calendarCard

        Layout.row: 1
        Layout.column: 1
        Layout.columnSpan: 3
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.preferredHeight: 276
        radius: root.theme.roundingExtraLarge
        theme: root.theme

        DashboardCalendar {
            anchors.fill: parent
            anchors.margins: root.theme.paddingLarge
            theme: root.theme
        }
    }

    // --- Resources ---------------------------------------------------------
    Tile {
        id: resourcesCard

        Layout.row: 1
        Layout.column: 4
        Layout.preferredWidth: 70
        Layout.fillHeight: true
        radius: root.theme.roundingLarge
        theme: root.theme

        Column {
            anchors.centerIn: parent
            spacing: root.theme.spacingLargeIncreased

            Repeater {
                model: [
                    { icon: "memory", value: System.cpuUsage, colour: root.theme.primary },
                    { icon: "memory_alt", value: System.memoryPercent, colour: root.theme.tertiary },
                    { icon: "hard_disk", value: System.diskPercent, colour: root.theme.secondary }
                ]

                Item {
                    required property var modelData

                    width: 42
                    height: 42

                    CircularProgress {
                        anchors.fill: parent
                        theme: root.theme
                        animate: root.animations
                        value: modelData.value / 100
                        strokeWidth: 6
                        fgColour: modelData.colour
                    }

                    MaterialIcon {
                        anchors.centerIn: parent
                        theme: root.theme
                        text: modelData.icon
                        iconSize: 18
                        color: modelData.colour
                    }
                }
            }
        }
    }
}
