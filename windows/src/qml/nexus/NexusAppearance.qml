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

    // Free colour picker state. Seeded from the stored custom accent so the
    // page opens on the colour actually in use — hue *and* saturation, or the
    // first drag would silently change a colour the user already picked. The
    // strips are built here rather than with QtQuick.Dialogs: that module is
    // not part of the portable deployment, and a native Win32 dialog looks
    // nothing like the rest of the shell.
    property real customHue: 0.6
    property real customSaturation: 0.72
    property real customLight: 0.6
    readonly property color pickedColour:
        Qt.hsla(customHue, customSaturation, customLight, 1)

    function applyCustomColour() {
        Platform.setShellSetting("appearance/accent", pickedColour.toString());
        Platform.setShellSetting("appearance/scheme", "custom");
    }

    property real shellOpacity: 0.85

    function applyShellOpacity(value) {
        const next = Math.round(Math.max(0.3, Math.min(1, value)) * 100) / 100;
        if (Math.abs(next - root.shellOpacity) < 0.004)
            return;
        root.shellOpacity = next;
        Platform.setShellSetting("appearance/opacity", next);
    }

    Component.onCompleted: {
        const c = root.theme.customAccent;
        root.customHue = c.hslHue >= 0 ? c.hslHue : 0.6;
        // A stored grey has no hue to show in the strips; keep them usable.
        root.customSaturation = Math.max(0.35, c.hslSaturation);
        root.customLight = c.hslLightness;

        const stored = Number(Platform.shellSetting("appearance/opacity", 0.85));
        root.shellOpacity = isNaN(stored) ? 0.85 : Math.max(0.3, Math.min(1, stored));
    }

    ColumnLayout {
        id: content

        width: root.width
        spacing: root.theme.spacingLarge

        StyledText {
            theme: root.theme
            role: "titleSmall"
            text: I18n.t("Fond d'écran")
        }

        StyledText {
            Layout.fillWidth: true
            theme: root.theme
            role: "labelSmall"
            color: root.theme.onSurfaceVariantColour
            wrapMode: Text.WordWrap
            text: Wallpaper.wallpapers.length === 0
                ? I18n.t("Aucune image trouvée dans les dossiers Windows.")
                : I18n.t("Cliquez sur une vignette : Windows applique l'image immédiatement.")
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
            Layout.preferredHeight: 1
            color: root.theme.outlineVariant
            opacity: 0.6
        }

        StyledText {
            theme: root.theme
            role: "titleSmall"
            text: I18n.t("Couleurs du shell")
        }

        StyledText {
            Layout.fillWidth: true
            theme: root.theme
            role: "labelSmall"
            color: root.theme.onSurfaceVariantColour
            wrapMode: Text.WordWrap
            text: "Teinte vraiment tout le shell : la barre, les panneaux, les "
                + "popouts, les cartes et les fonds suivent la couleur choisie. "
                + "Seules les applications Windows gardent leurs propres couleurs."
        }

        // One pill per preset. The whole grid is rebuilt when the palette
        // list changes, which only happens when the system accent moves.
        GridLayout {
            Layout.fillWidth: true
            columns: 3
            rowSpacing: root.theme.spacingSmall
            columnSpacing: root.theme.spacingSmall

            Repeater {
                model: root.theme.schemes

                Rectangle {
                    id: schemePill

                    required property var modelData

                    readonly property bool current: root.theme.scheme === modelData.key

                    Layout.fillWidth: true
                    Layout.preferredHeight: 54
                    radius: root.theme.roundingMedium
                    color: schemePill.current
                        ? root.theme.secondaryContainer : "transparent"
                    border.width: 1
                    border.color: schemePill.current
                        ? root.theme.primary : root.theme.outlineVariant

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: root.theme.spacingMedium
                        anchors.rightMargin: root.theme.spacingMedium
                        spacing: root.theme.spacingSmall

                        Rectangle {
                            Layout.preferredWidth: 22
                            Layout.preferredHeight: 22
                            radius: 11
                            color: schemePill.modelData.colour
                            border.width: 1
                            border.color: root.theme.outline
                            opacity: schemePill.current ? 1 : 0.85
                        }

                        StyledText {
                            Layout.fillWidth: true
                            theme: root.theme
                            role: "labelSmall"
                            weight: schemePill.current ? Font.DemiBold : Font.Normal
                            elide: Text.ElideRight
                            text: schemePill.modelData.label
                        }
                    }

                    StateLayer {
                        anchors.fill: parent
                        theme: root.theme
                        radius: parent.radius
                        onClicked: Platform.setShellSetting("appearance/scheme",
                            schemePill.modelData.key)
                    }
                }
            }
        }

        // Any hue, any brightness. Releasing a strip applies the colour as the
        // "Perso" scheme, and the whole shell repaints at once — the picker
        // doubles as the proof that the theme reaches every surface.
        RowLayout {
            Layout.fillWidth: true
            spacing: root.theme.spacingSmall

            Rectangle {
                Layout.preferredWidth: 24
                Layout.preferredHeight: 24
                radius: 12
                color: root.pickedColour
                border.width: 1
                border.color: root.theme.outline
            }

            StyledText {
                theme: root.theme
                role: "labelSmall"
                color: root.theme.onSurfaceVariantColour
                text: I18n.t("Couleur libre") + " — "
                    + root.pickedColour.toString().toUpperCase()
            }

            Item {
                Layout.fillWidth: true
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 24

            Rectangle {
                anchors.fill: parent
                radius: height / 2
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.000; color: Qt.hsla(0.000, root.customSaturation, 0.55, 1) }
                    GradientStop { position: 0.167; color: Qt.hsla(0.167, root.customSaturation, 0.55, 1) }
                    GradientStop { position: 0.333; color: Qt.hsla(0.333, root.customSaturation, 0.55, 1) }
                    GradientStop { position: 0.500; color: Qt.hsla(0.500, root.customSaturation, 0.55, 1) }
                    GradientStop { position: 0.667; color: Qt.hsla(0.667, root.customSaturation, 0.55, 1) }
                    GradientStop { position: 0.833; color: Qt.hsla(0.833, root.customSaturation, 0.55, 1) }
                    GradientStop { position: 1.000; color: Qt.hsla(1.000, root.customSaturation, 0.55, 1) }
                }
            }

            Rectangle {
                width: 8
                height: parent.height + 6
                y: -3
                radius: 4
                color: "transparent"
                border.width: 2
                border.color: root.theme.onSurfaceColour
                x: Math.max(0, Math.min(parent.width - width,
                    root.customHue * parent.width - width / 2))
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor

                function pick(mx) {
                    root.customHue = Math.max(0, Math.min(1, mx / width));
                }

                onPressed: pick(mouse.x)
                onPositionChanged: {
                    if (pressed)
                        pick(mouse.x);
                }
                onReleased: root.applyCustomColour()
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 14

            Rectangle {
                anchors.fill: parent
                radius: height / 2
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    // Black → the picked hue → white, matching the linear
                    // 0.04..1.0 lightness ramp the MouseArea reads back.
                    GradientStop { position: 0.000; color: "#000000" }
                    GradientStop { position: 0.531; color: Qt.hsla(root.customHue, root.customSaturation, 0.55, 1) }
                    GradientStop { position: 1.000; color: "#ffffff" }
                }
            }

            Rectangle {
                width: 6
                height: parent.height + 6
                y: -3
                radius: 3
                color: "transparent"
                border.width: 2
                border.color: root.theme.onSurfaceColour
                x: Math.max(0, Math.min(parent.width - width,
                    (root.customLight - 0.04) / 0.96 * parent.width - width / 2))
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor

                function pick(mx) {
                    root.customLight = 0.04
                        + 0.96 * Math.max(0, Math.min(1, mx / width));
                }

                onPressed: pick(mouse.x)
                onPositionChanged: {
                    if (pressed)
                        pick(mouse.x);
                }
                onReleased: root.applyCustomColour()
            }
        }

        StyledText {
            theme: root.theme
            role: "titleSmall"
            text: I18n.t("Opacité")
        }

        StyledText {
            Layout.fillWidth: true
            theme: root.theme
            role: "labelSmall"
            color: root.theme.onSurfaceVariantColour
            wrapMode: Text.WordWrap
            text: I18n.t("Le fond du shell laisse voir le fond d'écran. Les cartes des ")
                + "panneaux restent opaques pour que le texte reste lisible."
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: root.theme.spacingSmall

            M3Slider {
                id: opacitySlider

                Layout.fillWidth: true
                theme: root.theme
                onMoved: value => root.applyShellOpacity(value)
            }

            // M3Slider assigns `value` itself while dragging, which would kill
            // a plain binding; this re-syncs it whenever the slider is idle.
            Binding {
                target: opacitySlider
                property: "value"
                value: root.shellOpacity
                when: !opacitySlider.dragging
            }

            StyledText {
                Layout.preferredWidth: 48
                horizontalAlignment: Text.AlignRight
                theme: root.theme
                role: "labelSmall"
                color: root.theme.onSurfaceVariantColour
                text: Math.round(root.shellOpacity * 100) + " %"
            }
        }

        StyledText {
            theme: root.theme
            role: "titleSmall"
            text: I18n.t("Thème")
        }

        M3ListItem {
            Layout.fillWidth: true
            theme: root.theme
            icon: Platform.darkMode ? "dark_mode" : "light_mode"
            title: Platform.darkMode ? "Mode sombre" : "Mode clair"
            subtitle: I18n.t("Caelestia suit le thème des applications Windows")
            trailingText: "F8"
            onClicked: Qt.openUrlExternally("ms-settings:personalization")
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            M3Button {
                theme: root.theme
                text: I18n.t("Couleurs Windows")
                icon: "palette"
                variant: "tonal"
                onClicked: Qt.openUrlExternally("ms-settings:personalization-colors")
            }

            M3Button {
                theme: root.theme
                text: I18n.t("Arrière-plan Windows")
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
