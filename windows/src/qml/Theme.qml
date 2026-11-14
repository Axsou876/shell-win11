import QtQuick

QtObject {
    // Strict monochrome palette by default: pure black in dark mode, pure
    // white in light mode, with neutral greys only for hierarchy. The shell
    // follows the Windows app theme; F8 previews the other one.
    property bool dark: true

    // Accent scheme chosen in Nexus > Apparence. "monochrome" keeps the
    // original neutral palette. Every other preset — including the arbitrary
    // colour behind "custom" — drives the *whole* shell: the neutrals are
    // rebuilt in the accent's hue, so the bar frame, the panels, the popouts
    // and the cards become dark coloured glass instead of staying black.
    property string scheme: "monochrome"
    // Windows' own accent, used by the "windows" preset.
    property color systemAccent: "#ffb0ca"
    // Whatever the user picks in the colour dialog, used by "custom".
    property color customAccent: "#5b9dff"

    readonly property var schemes: [
        { key: "monochrome", label: "Monochrome", colour: dark ? "#ffffff" : "#000000" },
        // Palettes people already love in editors and terminals. `base` and
        // `sat` are the palette's own background metrics, so the derived
        // chrome lands on its real surface instead of a generic tint of the
        // accent: Catppuccin's #1e1e2e, Dracula's #282a36, Nord's #2e3440…
        { key: "catppuccin", label: "Catppuccin", colour: "#cba6f7", base: 0.149, sat: 0.21 },
        { key: "dracula", label: "Dracula", colour: "#bd93f9", base: 0.184, sat: 0.15 },
        { key: "nord", label: "Nord", colour: "#88c0d0", base: 0.216, sat: 0.16 },
        { key: "tokyo", label: "Tokyo", colour: "#7aa2f7", base: 0.125, sat: 0.19 },
        // Gruvbox's own background is neutral; a hint of warmth keeps the
        // chrome from reading as "no colour picked".
        { key: "gruvbox", label: "Gruvbox", colour: "#fe8019", base: 0.157, sat: 0.10 },
        { key: "rosepine", label: "Rosé", colour: "#eb6f92", base: 0.120, sat: 0.18 },
        { key: "everforest", label: "Everforest", colour: "#a7c080", base: 0.204, sat: 0.14 },
        { key: "kanagawa", label: "Kanagawa", colour: "#7e9cd8", base: 0.100, sat: 0.14 },
        { key: "solarized", label: "Solarized", colour: "#268bd2", base: 0.106, sat: 0.30 },
        { key: "onedark", label: "One Dark", colour: "#61afef", base: 0.180, sat: 0.13 },
        { key: "custom", label: "Perso", colour: customAccent },
        { key: "windows", label: "Windows", colour: systemAccent }
    ]

    readonly property color baseAccent: {
        for (let i = 0; i < schemes.length; ++i) {
            if (schemes[i].key === scheme)
                return schemes[i].colour;
        }
        return dark ? "#ffffff" : "#000000";
    }

    // A colour without a hue (pure white, pure black, a grey) cannot tint
    // anything, so it falls back to the neutral palette.
    readonly property bool coloured: scheme !== "monochrome"
        && baseAccent.hslSaturation > 0.02
    // The single hue every neutral is rebuilt from.
    readonly property real accentHue: baseAccent.hslHue < 0 ? 0 : baseAccent.hslHue
    // Swatch to show in the colour picker.
    readonly property color accentSwatch: coloured ? baseAccent
        : (dark ? "#ffffff" : "#000000")

    // Tint helper: overlay `colour` on `base` at `alpha` (0..1).
    function tint(base, colour, alpha) {
        return Qt.tint(base, Qt.rgba(colour.r, colour.g, colour.b, alpha));
    }

    // The active scheme's own background metrics, when it is a known palette.
    readonly property var schemeTuning: {
        for (let i = 0; i < schemes.length; ++i) {
            if (schemes[i].key === scheme)
                return schemes[i];
        }
        return {};
    }

    // The core of the coloured palette. `monoDark`/`monoLight` are the exact
    // hex values of the original strict palette; a scheme instead rebuilds the
    // same role as HSL of the accent's hue. A palette preset also carries its
    // real background lightness and saturation: the whole ramp shifts with
    // that anchor so the hierarchy survives while the chrome matches the
    // palette (0.130 is the generic base `surface` uses).
    function surfaceColour(monoDark, monoLight, darkL, lightL, darkS, lightS) {
        if (!coloured)
            return dark ? monoDark : monoLight;
        if (!dark)
            return Qt.hsla(accentHue, lightS, lightL, 1);
        const tuning = schemeTuning;
        const shift = tuning.base !== undefined ? tuning.base - 0.130 : 0;
        const saturation = tuning.sat !== undefined ? tuning.sat : darkS;
        return Qt.hsla(accentHue, saturation,
            Math.max(0.02, Math.min(0.98, darkL + shift)), 1);
    }

    readonly property color primary: coloured
        ? (dark ? Qt.lighter(baseAccent, 1.15) : Qt.darker(baseAccent, 1.35))
        : (dark ? "#ffffff" : "#000000")
    readonly property color onPrimaryColour: coloured
        ? (dark ? "#0d0d0d" : "#ffffff")
        : (dark ? "#000000" : "#ffffff")
    // Containers are hue-built too: a real tinted fill rather than a grey
    // washed with 20–38 % of the accent, which stayed grey on screen.
    readonly property color primaryContainer:
        surfaceColour("#1f1f1f", "#e6e6e6", 0.26, 0.80, 0.52, 0.46)
    readonly property color onPrimaryContainerColour:
        surfaceColour("#ffffff", "#000000", 0.90, 0.24, 0.60, 0.55)
    readonly property color secondary: coloured
        ? tint(dark ? "#d0d0d0" : "#333333", baseAccent, 0.45)
        : (dark ? "#d0d0d0" : "#333333")
    readonly property color onSecondaryColour: coloured
        ? (dark ? "#0d0d0d" : "#ffffff")
        : (dark ? "#000000" : "#ffffff")
    readonly property color secondaryContainer:
        surfaceColour("#2a2a2a", "#e0e0e0", 0.22, 0.84, 0.46, 0.42)
    readonly property color onSecondaryContainerColour:
        surfaceColour("#ffffff", "#000000", 0.91, 0.22, 0.52, 0.50)
    readonly property color tertiary: coloured
        ? tint(dark ? "#bdbdbd" : "#444444", baseAccent, 0.7)
        : (dark ? "#bdbdbd" : "#444444")
    readonly property color onTertiaryColour: coloured
        ? (dark ? "#0d0d0d" : "#ffffff")
        : (dark ? "#000000" : "#ffffff")
    readonly property color tertiaryContainer:
        surfaceColour("#333333", "#dddddd", 0.28, 0.86, 0.30, 0.34)
    readonly property color onTertiaryContainerColour:
        surfaceColour("#ffffff", "#000000", 0.90, 0.26, 0.42, 0.42)
    readonly property color error: dark ? "#ffffff" : "#000000"
    readonly property color onErrorColour: dark ? "#000000" : "#ffffff"
    // The shell chrome. A scheme rebuilds this whole ramp in its hue, so the
    // "black" of the bar, the panels and the popouts actually takes the
    // colour instead of staying #000 with a 5 % wash.
    //
    // Monochrome is not pure #000 anymore: on a dark wallpaper a black shell
    // is indistinguishable from the hole and only the white glyphs showed.
    // #161616 keeps the palette neutral while giving the bar, the frame and
    // the panels a surface you can actually see.
    readonly property color surface:
        surfaceColour("#161616", "#ffffff", 0.130, 0.945, 0.55, 0.50)
    readonly property color surfaceDim:
        surfaceColour("#121212", "#d9d9d9", 0.110, 0.900, 0.52, 0.50)
    readonly property color surfaceBright:
        surfaceColour("#303030", "#ffffff", 0.250, 1.000, 0.46, 0.35)
    readonly property color surfaceContainerLowest:
        surfaceColour("#0f0f0f", "#ffffff", 0.100, 0.975, 0.55, 0.45)
    readonly property color surfaceContainerLow:
        surfaceColour("#1a1a1a", "#f7f7f7", 0.150, 0.935, 0.54, 0.50)
    readonly property color surfaceContainer:
        surfaceColour("#1e1e1e", "#efefef", 0.185, 0.910, 0.52, 0.52)
    readonly property color surfaceContainerHigh:
        surfaceColour("#232323", "#e7e7e7", 0.220, 0.875, 0.50, 0.54)
    readonly property color surfaceContainerHighest:
        surfaceColour("#292929", "#dedede", 0.255, 0.845, 0.48, 0.55)
    readonly property color onSurfaceColour: dark ? "#ffffff" : "#000000"
    readonly property color surfaceVariant:
        surfaceColour("#333333", "#d0d0d0", 0.300, 0.820, 0.45, 0.52)
    readonly property color onSurfaceVariantColour:
        surfaceColour("#b3b3b3", "#4d4d4d", 0.720, 0.400, 0.30, 0.30)
    readonly property color inverseSurface: dark ? "#ffffff" : "#1a1a1a"
    readonly property color inverseOnSurface: dark ? "#000000" : "#ffffff"
    readonly property color outline:
        surfaceColour("#8a8a8a", "#6b6b6b", 0.600, 0.540, 0.32, 0.30)
    readonly property color outlineVariant:
        surfaceColour("#2e2e2e", "#d0d0d0", 0.300, 0.850, 0.42, 0.48)
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
