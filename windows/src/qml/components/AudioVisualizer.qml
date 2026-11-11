import QtQuick

// Radial audio visualiser matching CoverVisualiser.qml: 64 bars around the
// cover, driven only by the real FFT bands published by AudioCapture.
Item {
    id: root

    required property var theme

    property var levels: []
    property int barCount: 64
    property real coverSize: 200
    property real maxBarLength: 38

    implicitWidth: 300
    implicitHeight: 300

    onLevelsChanged: canvas.requestPaint()

    Canvas {
        id: canvas

        anchors.fill: parent
        antialiasing: true

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        onPaint: {
            const ctx = getContext("2d");
            ctx.reset();
            const w = width;
            const h = height;
            if (w <= 0 || h <= 0)
                return;

            const cx = w / 2;
            const cy = h / 2;
            const inner = root.coverSize / 2 + root.theme.spacingMedium;
            const count = Math.max(1, root.barCount);
            const values = root.levels || [];
            const available = values.length > 0;

            ctx.lineCap = "round";
            ctx.lineWidth = 4;
            ctx.strokeStyle = root.theme.primary;

            for (let i = 0; i < count; ++i) {
                const angle = -Math.PI / 2 + i * 2 * Math.PI / count;
                const value = available
                    ? Math.max(0.015, Math.min(1, values[i % values.length]))
                    : 0.015;
                const length = root.maxBarLength * value;
                const cos = Math.cos(angle);
                const sin = Math.sin(angle);

                ctx.beginPath();
                ctx.moveTo(cx + cos * inner, cy + sin * inner);
                ctx.lineTo(cx + cos * (inner + length), cy + sin * (inner + length));
                ctx.stroke();
            }
        }
    }
}
