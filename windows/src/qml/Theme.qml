import QtQuick

QtObject {
    // Strict monochrome palette: pure black in dark mode, pure white in light
    // mode, with neutral greys only for hierarchy. No coloured accent remains.
    // The shell follows the Windows app theme; F8 previews the other one.
    property bool dark: true

    readonly property color primary: dark ? "#ffffff" : "#000000"
    readonly property color onPrimaryColour: dark ? "#000000" : "#ffffff"
    readonly property color primaryContainer: dark ? "#1f1f1f" : "#e6e6e6"
    readonly property color onPrimaryContainerColour: dark ? "#ffffff" : "#000000"
    readonly property color secondary: dark ? "#d0d0d0" : "#333333"
    readonly property color onSecondaryColour: dark ? "#000000" : "#ffffff"
    readonly property color secondaryContainer: dark ? "#2a2a2a" : "#e0e0e0"
    readonly property color onSecondaryContainerColour: dark ? "#ffffff" : "#000000"
    readonly property color tertiary: dark ? "#bdbdbd" : "#444444"
    readonly property color onTertiaryColour: dark ? "#000000" : "#ffffff"
    readonly property color tertiaryContainer: dark ? "#333333" : "#dddddd"
    readonly property color onTertiaryContainerColour: dark ? "#ffffff" : "#000000"
    readonly property color error: dark ? "#ffffff" : "#000000"
    readonly property color onErrorColour: dark ? "#000000" : "#ffffff"
    readonly property color surface: dark ? "#000000" : "#ffffff"
    readonly property color surfaceDim: dark ? "#000000" : "#d9d9d9"
    readonly property color surfaceBright: dark ? "#262626" : "#ffffff"
    readonly property color surfaceContainerLowest: dark ? "#000000" : "#ffffff"
    readonly property color surfaceContainerLow: dark ? "#0a0a0a" : "#f7f7f7"
    readonly property color surfaceContainer: dark ? "#121212" : "#efefef"
    readonly property color surfaceContainerHigh: dark ? "#1c1c1c" : "#e7e7e7"
    readonly property color surfaceContainerHighest: dark ? "#262626" : "#dedede"
    readonly property color onSurfaceColour: dark ? "#ffffff" : "#000000"
    readonly property color surfaceVariant: dark ? "#2e2e2e" : "#d0d0d0"
    readonly property color onSurfaceVariantColour: dark ? "#b3b3b3" : "#4d4d4d"
    readonly property color inverseSurface: dark ? "#ffffff" : "#1a1a1a"
    readonly property color inverseOnSurface: dark ? "#000000" : "#ffffff"
    readonly property color outline: dark ? "#8a8a8a" : "#6b6b6b"
    readonly property color outlineVariant: dark ? "#2e2e2e" : "#d0d0d0"
    readonly property color shadow: "#000000"
    readonly property color scrim: "#000000"
    readonly property color surfaceTint: dark ? "#ffffff" : "#000000"
    readonly property color inversePrimary: dark ? "#000000" : "#ffffff"

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
    readonly property int popoutBatteryWidth: 250
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
