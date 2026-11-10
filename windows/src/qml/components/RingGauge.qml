import QtQuick
import ".."

// Circular performance gauge matching the dashboard reference. The value is a
// real percentage or memory ratio; no temperature is invented when Windows
// does not expose one.
Item {
    id: root

    required property var theme

    property real value: 0
    property string primaryText: ""
    property string secondaryText: ""
    property color colour: theme.primary
    property color trackColour: theme.surfaceContainerHighest

    implicitWidth: 190
    implicitHeight: 210

    Canvas {
        id: canvas

        anchors.fill: parent
        anchors.bottomMargin: 26

        onPaint: {
            const ctx = getContext("2d");
            ctx.reset();
            const w = width;
            const h = height;
            const cx = w / 2;
            const cy = h / 2;
            const radius = Math.min(w, h) / 2 - 14;
            const lineWidth = 9;
            const start = -Math.PI / 2;
            const end = start + Math.PI * 2;

            ctx.lineWidth = lineWidth;
            ctx.lineCap = "round";

            ctx.beginPath();
            ctx.arc(cx, cy, radius, start, end);
            ctx.strokeStyle = root.trackColour;
            ctx.stroke();

            const fraction = Math.max(0, Math.min(1, root.value / 100));
            if (fraction > 0.001) {
                ctx.beginPath();
                ctx.arc(cx, cy, radius, start, start + Math.PI * 2 * fraction);
                ctx.strokeStyle = root.colour;
                ctx.stroke();
            }

            // Dark origin caps, as in the reference rings.
            ctx.beginPath();
            ctx.arc(cx, cy, radius, start, start + 0.045);
            ctx.strokeStyle = root.theme.tertiary;
            ctx.lineWidth = lineWidth + 1;
            ctx.stroke();
        }

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }

    Column {
        anchors.centerIn: canvas
        anchors.verticalCenterOffset: -3
        spacing: 2

        StyledText {
            anchors.horizontalCenter: parent.horizontalCenter
            theme: root.theme
            role: "bodyLarge"
            pointSize: 30
            weight: Font.DemiBold
            text: root.primaryText
        }

        StyledText {
            anchors.horizontalCenter: parent.horizontalCenter
            theme: root.theme
            role: "labelSmall"
            color: root.theme.onSurfaceVariantColour
            text: root.secondaryText
        }
    }

    onValueChanged: canvas.requestPaint()
    onColourChanged: canvas.requestPaint()
    onTrackColourChanged: canvas.requestPaint()
}
