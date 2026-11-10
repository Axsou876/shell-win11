import QtQuick
import Caelestia.Platform 1.0
import ".."
import "../components"

Item {
    id: root

    required property var theme

    function iconUrl(path) {
        return path ? "file:///" + path.replace(/\\/g, "/") : "";
    }

    function formatTime(ms) {
        const total = Math.max(0, Math.floor(ms / 1000));
        const minutes = Math.floor(total / 60);
        const seconds = total % 60;
        return minutes + ":" + (seconds < 10 ? "0" : "") + seconds;
    }

    Row {
        anchors.fill: parent
        anchors.margins: root.theme.paddingLarge
        spacing: root.theme.spacingExtraLarge
        visible: Media.available

        Item {
            width: 260
            height: 260
            anchors.verticalCenter: parent.verticalCenter

            Canvas {
                id: visualizer

                anchors.fill: parent
                rotation: 0

                property bool active: Media.playing

                RotationAnimator on rotation {
                    running: visualizer.active
                    from: 0
                    to: 360
                    duration: 9000
                    loops: Animation.Infinite
                }

                onPaint: {
                    const ctx = getContext("2d");
                    ctx.reset();
                    const cx = width / 2;
                    const cy = height / 2;
                    const outer = Math.min(width, height) / 2 - 2;
                    const inner = outer - 12;
                    ctx.lineCap = "round";
                    ctx.lineWidth = 4;
                    for (let i = 0; i < 56; ++i) {
                        const angle = (i / 56) * Math.PI * 2;
                        ctx.beginPath();
                        const wobble = 0.85 + 0.15 * Math.sin(i * 1.7);
                        ctx.moveTo(cx + Math.cos(angle) * inner,
                            cy + Math.sin(angle) * inner);
                        ctx.lineTo(cx + Math.cos(angle) * (outer * wobble),
                            cy + Math.sin(angle) * (outer * wobble));
                        ctx.strokeStyle = i % 5 === 0
                            ? root.theme.tertiary : root.theme.primary;
                        ctx.stroke();
                    }
                }

                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()
                onActiveChanged: requestPaint()
            }

            RoundedClip {
                anchors.centerIn: parent
                width: 190
                height: 190
                radius: 95

                Rectangle {
                    anchors.fill: parent
                    color: root.theme.surfaceContainerHigh

                    Image {
                        anchors.fill: parent
                        source: root.iconUrl(Media.artworkPath)
                        visible: Media.artworkPath !== ""
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
                pointSize: 24
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

            Item { width: 1; height: 5 }

            Row {
                spacing: root.theme.spacingLarge

                M3Button {
                    theme: root.theme
                    icon: "skip_previous"
                    variant: "text"
                    implicitWidth: 44
                    implicitHeight: 40
                    enabled: Media.canPrevious
                    onClicked: Media.previous()
                }

                M3Button {
                    theme: root.theme
                    icon: Media.playing ? "pause" : "play_arrow"
                    variant: "filled"
                    implicitWidth: 56
                    implicitHeight: 44
                    enabled: Media.canPlayPause
                    onClicked: Media.playPause()
                }

                M3Button {
                    theme: root.theme
                    icon: "skip_next"
                    variant: "text"
                    implicitWidth: 44
                    implicitHeight: 40
                    enabled: Media.canNext
                    onClicked: Media.next()
                }
            }

            Item { width: 1; height: 5 }

            Rectangle {
                width: parent.width
                height: 4
                radius: 2
                color: root.theme.surfaceContainerHighest

                Rectangle {
                    width: Media.durationMs > 0
                        ? Math.min(1, Media.positionMs / Media.durationMs) * parent.width : 0
                    height: parent.height
                    radius: height / 2
                    color: root.theme.primary
                }
            }

            Row {
                width: parent.width

                StyledText {
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.onSurfaceVariantColour
                    text: root.formatTime(Media.positionMs)
                }

                Item { width: parent.width - 90; height: 1 }

                StyledText {
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.onSurfaceVariantColour
                    text: root.formatTime(Media.durationMs)
                }
            }

            Row {
                width: parent.width
                spacing: root.theme.spacingSmall

                Rectangle {
                    width: 70
                    height: 24
                    radius: 12
                    color: root.theme.secondaryContainer

                    Row {
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
                            text: Media.sourceApp.split("!").pop().replace(".exe", "")
                        }
                    }
                }

                StyledText {
                    anchors.verticalCenter: parent.verticalCenter
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.onSurfaceVariantColour
                    elide: Text.ElideRight
                    width: parent.width - 80
                    text: Audio.deviceName
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
            text: "Ouvre un lecteur compatible avec les contrôles média Windows."
        }
    }
}
