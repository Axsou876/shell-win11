import QtQuick

// Circular progress used by the dashboard resources, performance heroes and
// the home media card. The arc is driven by real service values only.
Item {
    id: root

    required property var theme

    property real value: 0
    property real startAngle: -90
    property real sweepAngle: 360
    property real strokeWidth: 6
    property color fgColour: theme.primary
    property color trackColour: Qt.alpha(theme.surfaceContainerHighest, 0.7)
    property real animatedValue: Math.max(0, Math.min(1, value))

    implicitWidth: 100
    implicitHeight: 100

    Behavior on animatedValue {
        NumberAnimation {
            duration: root.theme.durationDefaultSpatial
            easing.type: Easing.BezierSpline
            easing.bezierCurve: [0.38, 1.21, 0.22, 1, 1, 1]
        }
    }

    onAnimatedValueChanged: canvas.requestPaint()
    onStartAngleChanged: canvas.requestPaint()
    onSweepAngleChanged: canvas.requestPaint()
    onStrokeWidthChanged: canvas.requestPaint()
    onFgColourChanged: canvas.requestPaint()
    onTrackColourChanged: canvas.requestPaint()

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

            const thickness = Math.min(root.strokeWidth, Math.min(w, h) / 2);
            const radius = Math.min(w, h) / 2 - thickness / 2;
            const cx = w / 2;
            const cy = h / 2;
            const start = root.startAngle * Math.PI / 180;
            const sweep = root.sweepAngle * Math.PI / 180;
            const value = Math.max(0, Math.min(1, root.animatedValue));

            ctx.lineWidth = thickness;
            ctx.lineCap = "round";

            ctx.beginPath();
            ctx.arc(cx, cy, radius, start, start + sweep, false);
            ctx.strokeStyle = root.trackColour;
            ctx.stroke();

            if (value > 0.001) {
                ctx.beginPath();
                ctx.arc(cx, cy, radius, start, start + sweep * value, false);
                ctx.strokeStyle = root.fgColour;
                ctx.stroke();
            }
        }
    }
}
