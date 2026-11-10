import QtQuick

QtObject {
    // Light and dark M3 schemes derived from the original Caelestia palette.
    // The shell follows the Windows app theme; F8 previews the other one.
    property bool dark: true

    readonly property color primary: dark ? "#ffb0ca" : "#8b4a62"
    readonly property color onPrimaryColour: dark ? "#541d34" : "#ffffff"
    readonly property color primaryContainer: dark ? "#6f334a" : "#ffd9e3"
    readonly property color onPrimaryContainerColour: dark ? "#ffd9e3" : "#3a071e"
    readonly property color secondary: dark ? "#e2bdc7" : "#75565e"
    readonly property color onSecondaryColour: dark ? "#422932" : "#ffffff"
    readonly property color secondaryContainer: dark ? "#5a3f48" : "#ffd9e3"
    readonly property color onSecondaryContainerColour: dark ? "#ffd9e3" : "#2b151c"
    readonly property color tertiary: dark ? "#f0bc95" : "#7d5638"
    readonly property color onTertiaryColour: dark ? "#48290c" : "#ffffff"
    readonly property color tertiaryContainer: dark ? "#b58763" : "#ffdcc1"
    readonly property color onTertiaryContainerColour: dark ? "#48290c" : "#2d1600"
    readonly property color error: dark ? "#ffb4ab" : "#ba1a1a"
    readonly property color onErrorColour: dark ? "#690005" : "#ffffff"
    readonly property color surface: dark ? "#191114" : "#fcf5f3"
    readonly property color surfaceDim: dark ? "#191114" : "#dcd9d8"
    readonly property color surfaceBright: dark ? "#403739" : "#fcf5f3"
    readonly property color surfaceContainerLowest: dark ? "#130c0e" : "#ffffff"
    readonly property color surfaceContainerLow: dark ? "#22191c" : "#fcfef9"
    readonly property color surfaceContainer: dark ? "#261d20" : "#f0e9e7"
    readonly property color surfaceContainerHigh: dark ? "#31282a" : "#eae3e1"
    readonly property color surfaceContainerHighest: dark ? "#3c3235" : "#e4dddb"
    readonly property color onSurfaceColour: dark ? "#efdfe2" : "#1c1b1e"
    readonly property color surfaceVariant: dark ? "#514347" : "#d5c2c6"
    readonly property color onSurfaceVariantColour: dark ? "#d5c2c6" : "#514347"
    readonly property color inverseSurface: dark ? "#efdfe2" : "#322f31"
    readonly property color inverseOnSurface: dark ? "#372e30" : "#f5eff0"
    readonly property color outline: dark ? "#9e8c91" : "#837377"
    readonly property color outlineVariant: dark ? "#514347" : "#d5c2c6"
    readonly property color shadow: "#000000"
    readonly property color scrim: "#000000"
    readonly property color surfaceTint: dark ? "#ffb0ca" : "#8b4a62"
    readonly property color inversePrimary: dark ? "#8b4a62" : "#ffb0ca"

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
    readonly property int dashboardWidth: 854
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
