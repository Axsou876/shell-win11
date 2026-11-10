import QtQuick

// Small history chart used by the performance cards. Values are real samples
// from SystemService; the chart is empty until at least two samples exist.
Canvas {
    id: root

    property var values: []
    property color lineColor: "#ffffff"
    property color fillColor: lineColor
    property real maximum: 0

    onValuesChanged: requestPaint()
    onLineColorChanged: requestPaint()
    onMaximumChanged: requestPaint()

    onPaint: {
        const ctx = getContext("2d");
        ctx.reset();
        const w = width;
        const h = height;
        if (w <= 0 || h <= 0)
            return;

        ctx.lineWidth = 2;
        ctx.lineJoin = "round";
        ctx.lineCap = "round";
        if (!values || values.length < 2)
            return;

        let maxValue = maximum;
        if (maxValue <= 0) {
            for (let i = 0; i < values.length; ++i)
                maxValue = Math.max(maxValue, values[i]);
            maxValue = Math.max(1, maxValue * 1.15);
        }

        const step = w / (values.length - 1);
        ctx.beginPath();
        for (let i = 0; i < values.length; ++i) {
            const x = i * step;
            const y = h - Math.min(1, values[i] / maxValue) * (h - 4) - 2;
            if (i === 0)
                ctx.moveTo(x, y);
            else
                ctx.lineTo(x, y);
        }

        ctx.save();
        ctx.lineTo(w, h);
        ctx.lineTo(0, h);
        ctx.closePath();
        ctx.fillStyle = Qt.alpha(fillColor, 0.12);
        ctx.fill();
        ctx.restore();

        ctx.beginPath();
        for (let i = 0; i < values.length; ++i) {
            const x = i * step;
            const y = h - Math.min(1, values[i] / maxValue) * (h - 4) - 2;
            if (i === 0)
                ctx.moveTo(x, y);
            else
                ctx.lineTo(x, y);
        }
        ctx.strokeStyle = lineColor;
        ctx.stroke();
    }
}
