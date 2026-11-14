import QtQuick

// Keeps a shell window rendering at the display refresh instead of on demand.
//
// Qt Quick only paints when something changed, which is why an idle shell sits
// at a handful of frames per second. Asking for a frame every 7 ms turns the
// window into a continuous 144 Hz surface: the swap chain still paces it at
// the monitor's refresh, so the extra requests are coalesced, not drawn twice.
//
// The cost is real — identical pixels are rebuilt ~144 times a second — so the
// driver is only installed when Nexus > Système > « Rendu à 144 fps » is on.
Timer {
    id: root

    required property var targetWindow
    property bool active: false

    interval: 7
    repeat: true
    running: active && targetWindow !== null
    onTriggered: targetWindow.update()
}
