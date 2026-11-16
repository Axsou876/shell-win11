pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Caelestia.Platform 1.0
import ".."
import "../components"

// Media tab reconstructed from Media.qml / media/*:
// BackgroundShapes, CoverVisualiser (real WASAPI/FFT bands), Details and the
// lyrics section. The layout follows the Linux 300 / fill / 300 columns.
Item {
    id: root

    required property var theme

    property bool pageActive: false

    readonly property bool available: Media.available

    // --- Playhead -----------------------------------------------------------
    // MediaService polls GSMTC once per second, so `Media.positionMs` arrives
    // in whole-second steps and the slider used to jump once a second while
    // the music played. The playhead below extrapolates from the last sample
    // and re-anchors on every fresh one: both the elapsed time and the slider
    // then move every frame, and a seek still lands immediately.
    property real anchorMs: 0
    property double anchorAt: 0
    property real playheadMs: 0

    readonly property real progress: Media.durationMs > 0
        ? Math.max(0, Math.min(1, playheadMs / Media.durationMs)) : 0

    function anchorPlayhead() {
        anchorMs = Media.positionMs;
        anchorAt = Date.now();
        playheadMs = Media.positionMs;
    }

    // Derived properties: a Q_PROPERTY cannot be watched individually here,
    // every Media change emits the same `changed` signal. Re-reading the value
    // through a derived property gives back a per-field change handler.
    property real mediaSample: Media.positionMs
    property bool mediaPlaying: Media.playing

    onMediaSampleChanged: anchorPlayhead()
    onMediaPlayingChanged: anchorPlayhead()

    FrameAnimation {
        running: root.pageActive && root.mediaPlaying && Media.durationMs > 0
        onTriggered: root.playheadMs = Math.min(Media.durationMs,
            root.anchorMs + (Date.now() - root.anchorAt))
    }

    // --- Lyrics -------------------------------------------------------------
    // Index of the line being sung, by binary search over the whole track: it
    // is recomputed on every playhead frame, so the highlight follows the
    // singing instead of the one-second poll.
    readonly property int activeLine: {
        const lines = Lyrics.lines;
        if (!Lyrics.synced || lines.length === 0)
            return -1;
        const now = root.playheadMs;
        let low = 0;
        let high = lines.length - 1;
        let found = -1;
        while (low <= high) {
            const middle = (low + high) >> 1;
            if (lines[middle].time <= now) {
                found = middle;
                low = middle + 1;
            } else {
                high = middle - 1;
            }
        }
        return found;
    }

    // The service ignores a repeat of the same track, so this is safe to call
    // on every signal and every time the tab opens.
    function requestLyrics() {
        if (!root.pageActive || !root.available || Media.title === "")
            return;
        Lyrics.request(Media.title, Media.artist, Media.album, Media.durationMs);
    }

    property string trackKey: Media.title + "\u001f" + Media.artist

    onTrackKeyChanged: requestLyrics()

    function scrollLyrics(lineY) {
        lyricsFlick.contentY = Math.max(0, Math.min(
            lyricsFlick.contentHeight - lyricsFlick.height,
            lineY - lyricsFlick.height * 0.42));
    }

    clip: true

    onPageActiveChanged: {
        AudioCapture.setActive(pageActive);
        if (pageActive)
            requestLyrics();
    }
    Component.onCompleted: {
        AudioCapture.setActive(pageActive);
        requestLyrics();
    }
    Component.onDestruction: AudioCapture.setActive(false)

    function timeText(ms) {
        if (ms <= 0)
            return "0:00";
        const total = Math.floor(ms / 1000);
        const minutes = Math.floor(total / 60);
        const seconds = total % 60;
        return minutes + ":" + (seconds < 10 ? "0" : "") + seconds;
    }

    // --- Decorative monochrome background shapes (drifting when playing) ---
    Item {
        id: background

        anchors.fill: parent

        Repeater {
            id: shapes

            model: 14

            Rectangle {
                id: shape

                required property int index

                property real vx: (4 + Math.random() * 14) * (Math.random() < 0.5 ? -1 : 1)
                property real vy: (4 + Math.random() * 14) * (Math.random() < 0.5 ? -1 : 1)

                width: 36 + (index / 14) * 88
                height: width
                radius: index % 3 === 0 ? width / 2 : width * 0.22
                rotation: Math.random() * 360
                color: [root.theme.primaryContainer, root.theme.secondaryContainer,
                    root.theme.tertiaryContainer, root.theme.outlineVariant][index % 4]
                opacity: root.theme.dark ? 0.12 : 0.2

                Component.onCompleted: {
                    x = Math.random() * Math.max(1, background.width - width);
                    y = Math.random() * Math.max(1, background.height - height);
                }

                Behavior on opacity {
                    NumberAnimation {
                        duration: root.theme.durationSlowEffects
                    }
                }
            }
        }

        FrameAnimation {
            running: root.pageActive && Media.playing && background.width > 0
            onTriggered: {
                const dt = frameTime;
                for (let i = 0; i < shapes.count; ++i) {
                    const item = shapes.itemAt(i);
                    if (!item)
                        continue;
                    item.x += item.vx * dt;
                    item.y += item.vy * dt;
                    item.rotation += item.vx * dt * 0.4;
                    if (item.x + item.width < 0)
                        item.x = background.width;
                    else if (item.x > background.width)
                        item.x = -item.width;
                    if (item.y + item.height < 0)
                        item.y = background.height;
                    else if (item.y > background.height)
                        item.y = -item.height;
                }
            }
        }
    }

    // --- Content -----------------------------------------------------------
    Item {
        id: contentRoot

        anchors.fill: parent
        opacity: root.pageActive ? 1 : 0
        scale: root.pageActive ? 1 : 0.985

        Behavior on opacity {
            NumberAnimation {
                duration: root.theme.durationDefaultEffects
                easing.type: Easing.BezierSpline
                easing.bezierCurve: [0.34, 0.8, 0.34, 1, 1, 1]
            }
        }

        Behavior on scale {
            NumberAnimation {
                duration: root.theme.durationDefaultSpatial
                easing.type: Easing.BezierSpline
                easing.bezierCurve: [0.38, 1.21, 0.22, 1, 1, 1]
            }
        }

        RowLayout {
            anchors.fill: parent
            spacing: root.theme.spacingExtraLarge
            visible: root.available

            // Cover visualiser, 300 px as mediaSectionWidth.
            Item {
                id: coverSection

                Layout.preferredWidth: 300
                Layout.fillHeight: true

                AudioVisualizer {
                    anchors.centerIn: parent
                    width: 300
                    height: 300
                    theme: root.theme
                    levels: AudioCapture.bands
                    barCount: 64
                    coverSize: 200
                    maxBarLength: 38
                }

                RoundedClip {
                    id: cover

                    anchors.centerIn: parent
                    width: 200
                    height: 200
                    radius: width / 2
                    color: root.theme.surfaceContainerHighest

                    Image {
                        id: coverImage

                        anchors.fill: parent
                        source: Media.artworkPath !== ""
                            ? "file:///" + Media.artworkPath.replace(/\\/g, "/") : ""
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        opacity: 1
                        scale: 1

                        onSourceChanged: {
                            opacity = 0;
                            scale = 0.97;
                        }

                        onStatusChanged: {
                            if (status === Image.Ready) {
                                opacity = 1;
                                scale = 1;
                            }
                        }

                        Behavior on opacity {
                            NumberAnimation {
                                duration: root.theme.durationDefaultEffects
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [0.34, 0.8, 0.34, 1, 1, 1]
                            }
                        }

                        Behavior on scale {
                            NumberAnimation {
                                duration: root.theme.durationDefaultSpatial
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [0.38, 1.21, 0.22, 1, 1, 1]
                            }
                        }
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
            }

            // Details, fills the centre column.
            Item {
                id: details

                Layout.fillWidth: true
                Layout.fillHeight: true

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
                        animate: true
                        elide: Text.ElideRight
                        maximumLineCount: 2
                        wrapMode: Text.WordWrap
                        text: Media.title !== "" ? Media.title : I18n.t("Titre inconnu")
                    }

                    StyledText {
                        width: parent.width
                        theme: root.theme
                        role: "titleSmall"
                        color: root.theme.onSurfaceVariantColour
                        animate: true
                        elide: Text.ElideRight
                        text: Media.artist !== "" ? Media.artist : "Artiste inconnu"
                    }

                    StyledText {
                        width: parent.width
                        theme: root.theme
                        role: "titleSmall"
                        color: root.theme.secondary
                        animate: true
                        elide: Text.ElideRight
                        text: Media.album !== "" ? Media.album : "Album inconnu"
                    }

                    Item { width: 1; height: 12 }

                    RowLayout {
                        width: parent.width
                        spacing: root.theme.spacingSmall

                        StyledText {
                            Layout.preferredWidth: 42
                            Layout.alignment: Qt.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                            theme: root.theme
                            role: "labelSmall"
                            color: root.theme.onSurfaceVariantColour
                            text: root.timeText(root.playheadMs)
                        }

                        M3Slider {
                            id: positionSlider

                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                            theme: root.theme
                            value: root.progress
                            enabled: Media.canSeek && Media.durationMs > 0
                            onMoved: value => {
                                if (dragging && Media.canSeek && Media.durationMs > 0)
                                    Media.seek(Math.round(value * Media.durationMs));
                            }
                        }

                        StyledText {
                            Layout.preferredWidth: 42
                            Layout.alignment: Qt.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                            theme: root.theme
                            role: "labelSmall"
                            color: root.theme.onSurfaceVariantColour
                            text: root.timeText(Media.durationMs)
                        }
                    }

                    Binding {
                        target: positionSlider
                        property: "value"
                        value: root.progress
                        when: !positionSlider.dragging
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

            // Lyrics / selector column, 300 px as mediaSectionWidth.
            Rectangle {
                id: lyrics

                Layout.preferredWidth: 300
                Layout.fillHeight: true
                radius: root.theme.roundingLarge
                color: root.theme.surfaceContainer
                border.width: 1
                border.color: Qt.alpha(root.theme.outlineVariant, 0.25)

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: root.theme.paddingLarge
                    spacing: root.theme.spacingMedium

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: root.theme.spacingSmall

                        MaterialIcon {
                            Layout.alignment: Qt.AlignVCenter
                            theme: root.theme
                            text: "lyrics"
                            iconSize: 20
                            color: root.theme.primary
                        }

                        StyledText {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                            theme: root.theme
                            role: "titleSmall"
                            weight: Font.DemiBold
                            text: "Paroles"
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        // Synced lyrics from lrclib: the line being sung is
                        // highlighted and kept centred. `interactive` stays
                        // false so a stray wheel over the tab never fights the
                        // auto-scroll.
                        Flickable {
                            id: lyricsFlick

                            anchors.fill: parent
                            visible: Lyrics.status === "ready" && Lyrics.synced
                            contentWidth: width
                            contentHeight: lyricColumn.height
                            clip: true
                            interactive: false
                            boundsBehavior: Flickable.StopAtBounds

                            Behavior on contentY {
                                NumberAnimation {
                                    duration: root.theme.durationDefaultSpatial
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [0.38, 1.21, 0.22, 1, 1, 1]
                                }
                            }

                            Column {
                                id: lyricColumn

                                width: lyricsFlick.width
                                topPadding: lyricsFlick.height * 0.42
                                bottomPadding: lyricsFlick.height * 0.58
                                spacing: root.theme.spacingSmall

                                Repeater {
                                    model: Lyrics.lines

                                    onCountChanged: lyricsFlick.contentY = 0

                                    delegate: StyledText {
                                        id: lyricLine

                                        required property var modelData
                                        required property int index

                                        readonly property bool current: index === root.activeLine

                                        width: lyricColumn.width
                                        horizontalAlignment: Text.AlignHCenter
                                        wrapMode: Text.WordWrap
                                        theme: root.theme
                                        role: "bodyMedium"
                                        text: lyricLine.modelData.text
                                        color: lyricLine.current
                                            ? root.theme.primary : root.theme.onSurfaceVariantColour
                                        opacity: lyricLine.current ? 1 : 0.6
                                        scale: lyricLine.current ? 1.04 : 1

                                        onCurrentChanged: {
                                            if (current)
                                                root.scrollLyrics(lyricLine.y);
                                        }

                                        Behavior on opacity {
                                            NumberAnimation {
                                                duration: root.theme.durationFastEffects
                                            }
                                        }

                                        Behavior on scale {
                                            NumberAnimation {
                                                duration: root.theme.durationDefaultSpatial
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // Plain lyrics: no timestamps to follow, so they scroll
                        // by hand.
                        Flickable {
                            anchors.fill: parent
                            visible: Lyrics.status === "ready" && !Lyrics.synced
                                && Lyrics.plainLines.length > 0
                            contentWidth: width
                            contentHeight: plainColumn.height
                            clip: true
                            boundsBehavior: Flickable.StopAtBounds

                            Column {
                                id: plainColumn

                                width: parent.width
                                topPadding: root.theme.spacingSmall
                                spacing: root.theme.spacingExtraSmall

                                Repeater {
                                    model: Lyrics.plainLines

                                    delegate: StyledText {
                                        required property var modelData

                                        width: plainColumn.width
                                        theme: root.theme
                                        role: "bodyMedium"
                                        color: root.theme.onSurfaceVariantColour
                                        wrapMode: Text.WordWrap
                                        text: modelData
                                    }
                                }
                            }
                        }

                        // Loading / none / error / instrumental.
                        Column {
                            anchors.centerIn: parent
                            width: parent.width
                            spacing: root.theme.spacingSmall
                            visible: Lyrics.status !== "ready"
                                || (Lyrics.plainLines.length === 0 && !Lyrics.synced)

                            MaterialIcon {
                                anchors.horizontalCenter: parent.horizontalCenter
                                theme: root.theme
                                text: Lyrics.status === "loading" ? "hourglass_top"
                                    : Lyrics.status === "error" ? "cloud_off"
                                    : Lyrics.instrumental ? "music_off" : "lyrics"
                                iconSize: 42
                                color: root.theme.onSurfaceVariantColour
                                opacity: Lyrics.status === "loading" ? 0.5 : 0.7
                            }

                            StyledText {
                                anchors.horizontalCenter: parent.horizontalCenter
                                theme: root.theme
                                role: "bodyMedium"
                                weight: Font.DemiBold
                                text: Lyrics.status === "loading" ? "Recherche des paroles…"
                                    : Lyrics.status === "error" ? "Recherche impossible"
                                    : Lyrics.instrumental ? "Instrumental"
                                    : "Paroles introuvables"
                            }

                            StyledText {
                                width: parent.width
                                theme: root.theme
                                role: "labelSmall"
                                color: root.theme.onSurfaceVariantColour
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.WordWrap
                                text: Lyrics.status === "error" ? Lyrics.error
                                    : Lyrics.status === "loading" ? "lrclib.net"
                                    : Lyrics.instrumental ? "Ce titre n'a pas de paroles."
                                    : "Aucune correspondance sur lrclib.net pour ce titre."
                            }
                        }
                    }

                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        width: selectorRow.implicitWidth + root.theme.paddingMedium * 2
                        height: 28
                        radius: 14
                        color: root.theme.secondaryContainer

                        Row {
                            id: selectorRow

                            anchors.centerIn: parent
                            spacing: 4

                            MaterialIcon {
                                anchors.verticalCenter: parent.verticalCenter
                                theme: root.theme
                                text: "music_note"
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
                                    : "Aucun lecteur"
                            }
                        }
                    }
                }
            }
        }

        // Empty state.
        Column {
            anchors.centerIn: parent
            spacing: root.theme.spacingMedium
            visible: !root.available
            opacity: root.pageActive ? 1 : 0

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
                text: I18n.t("Aucune lecture")
            }

            StyledText {
                anchors.horizontalCenter: parent.horizontalCenter
                theme: root.theme
                role: "bodyMedium"
                color: root.theme.onSurfaceVariantColour
                text: I18n.t("Lancez un média compatible GSMTC pour l’afficher ici.")
            }
        }
    }
}
