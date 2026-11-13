import QtQuick

QtObject {
    // Strict monochrome palette: pure black in dark mode, pure white in light
    // mode, with neutral greys only for hierarchy. No coloured accent remains.
    // The shell follows the Windows app theme; F8 previews the other one.
    property bool dark: true

    // Accent scheme chosen in Nexus > Apparence. "monochrome" keeps the
    // original strict palette; the other presets tint the accent roles *and*
    // wash the neutral surfaces with the same colour, so the whole shell
    // follows the scheme while text contrast stays intact.
    property string scheme: "monochrome"
    // Windows' own accent, used by the "windows" preset.
    property color systemAccent: "#ffb0ca"

    readonly property var schemes: [
        { key: "monochrome", label: "Monochrome", colour: dark ? "#ffffff" : "#000000" },
        { key: "blue", label: "Bleu", colour: "#5b9dff" },
        { key: "violet", label: "Violet", colour: "#b18cff" },
        { key: "green", label: "Vert", colour: "#4fd18b" },
        { key: "amber", label: "Ambre", colour: "#f0b45a" },
        { key: "pink", label: "Rose", colour: "#f47fb4" },
        { key: "cyan", label: "Cyan", colour: "#46c8d8" },
        { key: "red", label: "Rouge", colour: "#f27272" },
        { key: "windows", label: "Windows", colour: systemAccent }
    ]

    readonly property bool coloured: scheme !== "monochrome"
    readonly property color baseAccent: {
        for (let i = 0; i < schemes.length; ++i) {
            if (schemes[i].key === scheme)
                return schemes[i].colour;
        }
        return dark ? "#ffffff" : "#000000";
    }

    // Tint helper: overlay `colour` on `base` at `alpha` (0..1).
    function tint(base, colour, alpha) {
        return Qt.tint(base, Qt.rgba(colour.r, colour.g, colour.b, alpha));
    }

    // Surfaces follow the chosen scheme too, not just the accent roles: the
    // whole shell (bar, panels, popouts, cards) takes a faint wash of the
    // accent. The alphas stay low because the design depends on near-black /
    // near-white surfaces for contrast; anything stronger greys out the text.
    function tinted(base, alpha) {
        return coloured ? tint(base, baseAccent, alpha) : base;
    }

    function accentTone(level) {
        // level < 1 darkens toward the light surface, > 1 lightens for dark
        // mode. The presets are mid-tone, so both directions stay readable.
        return dark ? Qt.lighter(baseAccent, level) : Qt.darker(baseAccent, level);
    }

    readonly property color primary: coloured
        ? (dark ? Qt.lighter(baseAccent, 1.15) : Qt.darker(baseAccent, 1.35))
        : (dark ? "#ffffff" : "#000000")
    readonly property color onPrimaryColour: coloured
        ? (dark ? "#0d0d0d" : "#ffffff")
        : (dark ? "#000000" : "#ffffff")
    readonly property color primaryContainer: coloured
        ? tint(dark ? "#1f1f1f" : "#e6e6e6", baseAccent, dark ? 0.38 : 0.32)
        : (dark ? "#1f1f1f" : "#e6e6e6")
    readonly property color onPrimaryContainerColour: coloured
        ? accentTone(dark ? 1.55 : 1.9)
        : (dark ? "#ffffff" : "#000000")
    readonly property color secondary: coloured
        ? tint(dark ? "#d0d0d0" : "#333333", baseAccent, 0.45)
        : (dark ? "#d0d0d0" : "#333333")
    readonly property color onSecondaryColour: coloured
        ? (dark ? "#0d0d0d" : "#ffffff")
        : (dark ? "#000000" : "#ffffff")
    readonly property color secondaryContainer: coloured
        ? tint(dark ? "#2a2a2a" : "#e0e0e0", baseAccent, dark ? 0.26 : 0.26)
        : (dark ? "#2a2a2a" : "#e0e0e0")
    readonly property color onSecondaryContainerColour: coloured
        ? accentTone(dark ? 1.6 : 2.0)
        : (dark ? "#ffffff" : "#000000")
    readonly property color tertiary: coloured
        ? tint(dark ? "#bdbdbd" : "#444444", baseAccent, 0.7)
        : (dark ? "#bdbdbd" : "#444444")
    readonly property color onTertiaryColour: coloured
        ? (dark ? "#0d0d0d" : "#ffffff")
        : (dark ? "#000000" : "#ffffff")
    readonly property color tertiaryContainer: coloured
        ? tint(dark ? "#333333" : "#dddddd", baseAccent, dark ? 0.2 : 0.22)
        : (dark ? "#333333" : "#dddddd")
    readonly property color onTertiaryContainerColour: coloured
        ? accentTone(dark ? 1.7 : 2.1)
        : (dark ? "#ffffff" : "#000000")
    readonly property color error: dark ? "#ffffff" : "#000000"
    readonly property color onErrorColour: dark ? "#000000" : "#ffffff"
    readonly property color surface: tinted(dark ? "#000000" : "#ffffff", dark ? 0.05 : 0.03)
    readonly property color surfaceDim: tinted(dark ? "#000000" : "#d9d9d9", dark ? 0.045 : 0.035)
    readonly property color surfaceBright: tinted(dark ? "#262626" : "#ffffff", dark ? 0.1 : 0.05)
    readonly property color surfaceContainerLowest: tinted(dark ? "#000000" : "#ffffff", dark ? 0.035 : 0.025)
    readonly property color surfaceContainerLow: tinted(dark ? "#0a0a0a" : "#f7f7f7", dark ? 0.055 : 0.035)
    readonly property color surfaceContainer: tinted(dark ? "#121212" : "#efefef", dark ? 0.07 : 0.045)
    readonly property color surfaceContainerHigh: tinted(dark ? "#1c1c1c" : "#e7e7e7", dark ? 0.085 : 0.055)
    readonly property color surfaceContainerHighest: tinted(dark ? "#262626" : "#dedede", dark ? 0.1 : 0.065)
    readonly property color onSurfaceColour: dark ? "#ffffff" : "#000000"
    readonly property color surfaceVariant: tinted(dark ? "#2e2e2e" : "#d0d0d0", dark ? 0.12 : 0.08)
    readonly property color onSurfaceVariantColour: dark ? "#b3b3b3" : "#4d4d4d"
    readonly property color inverseSurface: dark ? "#ffffff" : "#1a1a1a"
    readonly property color inverseOnSurface: dark ? "#000000" : "#ffffff"
    readonly property color outline: tinted(dark ? "#8a8a8a" : "#6b6b6b", dark ? 0.08 : 0.06)
    readonly property color outlineVariant: tinted(dark ? "#2e2e2e" : "#d0d0d0", dark ? 0.1 : 0.07)
    readonly property color shadow: "#000000"
    readonly property color scrim: "#000000"
    readonly property color surfaceTint: coloured ? primary : (dark ? "#ffffff" : "#000000")
    readonly property color inversePrimary: coloured
        ? (dark ? Qt.darker(baseAccent, 1.35) : Qt.lighter(baseAccent, 1.15))
        : (dark ? "#000000" : "#ffffff")

    // Names used by the Phase 1 prototype; kept for compatibility.
    readonly property color primaryText: onPrimaryColour
    readonly property color text: onSurfaceColour
    readonly property color textVariant: onSurfaceVariantColour
    readonly property color accent: primary

    readonly property string bodyFont: "Google Sans Flex"
    readonly property string iconFont: "Material Symbols Rounded"
    readonly property string clockFont: "Google Sans Flex"
    readonly property string workspaceFont: "Rubik"
    readonly property string monoFont: "CaskaydiaCove NF"

    // Original Caelestia geometry / tokens.
    readonly property int barInnerWidth: 40
    readonly property int barPadding: 10
    readonly property int barRailWidth: barInnerWidth + barPadding * 2
    readonly property int barVPadding: 16
    readonly property int borderThickness: 10
    readonly property int borderRounding: 25
    readonly property int borderSmoothing: 20
    readonly property real shadowOpacity: 0.7
    readonly property real appearanceDeformScale: 1

    // Large shell surfaces (Caelestia tokens).
    readonly property int sidebarWidth: 430
    readonly property int dashboardWidth: 1000
    readonly property int dashboardHeight: 560
    readonly property int launcherWidth: 640
    readonly property int launcherHeight: 520
    readonly property int launcherBottomMargin: 16
    readonly property int popoutWidth: 320
    readonly property int popoutNetworkWidth: 320
    readonly property int popoutAudioWidth: 320
    readonly property int popoutBluetoothWidth: 320
    // The original battery popout is the narrowest of the bar popouts. Our
    // footer adds "Paramètres d'alimentation", which does not fit in 250 px.
    readonly property int popoutBatteryWidth: 300
    readonly property int popoutWindowWidth: 340
    readonly property int popoutTrayWidth: 300
    readonly property int popoutHeight: 192

    function popoutWidthFor(name) {
        if (name === "network")
            return popoutNetworkWidth;
        if (name === "audio")
            return popoutAudioWidth;
        if (name === "bluetooth")
            return popoutBluetoothWidth;
        if (name === "battery")
            return popoutBatteryWidth;
        if (name === "window")
            return popoutWindowWidth;
        if (name === "tray")
            return popoutTrayWidth;
        return popoutWidth;
    }
    readonly property int panelRadius: roundingExtraLarge

    readonly property int iconSmall: 20
    readonly property int iconMedium: 24
    readonly property int iconLarge: 32
    readonly property int statusIconSize: 20
    readonly property int trayIconSize: 24

    readonly property int spacingExtraSmall: 4
    readonly property int spacingSmall: 8
    readonly property int spacingMedium: 12
    readonly property int spacingLarge: 16
    readonly property int spacingLargeIncreased: 20
    readonly property int spacingExtraLarge: 28

    readonly property int paddingExtraSmall: 4
    readonly property int paddingSmall: 8
    readonly property int paddingMedium: 12
    readonly property int paddingLarge: 16
    readonly property int paddingLargeIncreased: 20
    readonly property int paddingExtraLarge: 28

    readonly property int roundingExtraSmall: 4
    readonly property int roundingSmall: 8
    readonly property int roundingMedium: 12
    readonly property int roundingLarge: 16
    readonly property int roundingLargeIncreased: 20
    readonly property int roundingExtraLarge: 28
    readonly property int roundingExtraLargeIncreased: 32
    readonly property int roundingFull: 999

    // Original animation durations (tokens.hpp).
    readonly property int durationSmall: 200
    readonly property int durationNormal: 400
    readonly property int durationLarge: 600
    readonly property int durationExtraLarge: 1000
    readonly property int durationFastSpatial: 350
    readonly property int durationDefaultSpatial: 500
    readonly property int durationSlowSpatial: 650
    readonly property int durationFastEffects: 150
    readonly property int durationDefaultEffects: 200
    readonly property int durationSlowEffects: 300

    // Original Caelestia typography (point sizes).
    readonly property real bodySmallPointSize: 12
    readonly property real bodyMediumPointSize: 14
    readonly property real bodyLargePointSize: 16
    readonly property real titleSmallPointSize: 14
    readonly property real labelSmallPointSize: 11
    readonly property real clockPointSize: 13.2
    readonly property int variableRound: 25

    function layer(colour, alpha) {
        return Qt.alpha(colour, alpha);
    }
}
