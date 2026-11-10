import QtQuick

// Spring deformation tracker ported from Caelestia's BlobRect physics.
// It follows the panel centre and produces a small axis-aligned stretch/
// compress matrix. The same matrix is applied inverted to the blob SDF and
// forward to the panel content. The frame loop stops as soon as the spring
// is at rest, so an idle open panel costs no animation work.
Item {
    id: root

    visible: false
    width: 0
    height: 0

    property bool enabled: false
    property point centre: Qt.point(0, 0)
    property real stiffness: 200
    property real damping: 16
    property real deformScale: 0.00001

    property real m00: 1
    property real m11: 1
    property real v00: 0
    property real v11: 0

    property real prevX: 0
    property real prevY: 0
    property double prevTime: 0
    property bool hasPrev: false

    readonly property real scaleX: m00
    readonly property real scaleY: m11
    readonly property vector4d inverse: Qt.vector4d(
        1 / Math.max(0.01, m00), 0, 0, 1 / Math.max(0.01, m11))

    function reset() {
        m00 = 1;
        m11 = 1;
        v00 = 0;
        v11 = 0;
        hasPrev = false;
        frameAnim.running = false;
    }

    function wake() {
        if (!enabled)
            return;
        if (!frameAnim.running) {
            hasPrev = false;
            frameAnim.running = true;
        }
    }

    onEnabledChanged: {
        if (enabled)
            wake();
        else
            reset();
    }

    onCentreChanged: wake()

    FrameAnimation {
        id: frameAnim

        running: false
        onTriggered: root.step()
    }

    function step() {
        if (!enabled) {
            reset();
            return;
        }

        const now = Date.now();
        if (!hasPrev) {
            prevX = centre.x;
            prevY = centre.y;
            prevTime = now;
            hasPrev = true;
            return;
        }

        let dt = (now - prevTime) / 1000;
        prevTime = now;
        if (dt < 0.001 || dt > 0.1) {
            prevX = centre.x;
            prevY = centre.y;
            return;
        }

        const vx = (centre.x - prevX) / dt;
        const vy = (centre.y - prevY) / dt;
        prevX = centre.x;
        prevY = centre.y;
        const speed = Math.sqrt(vx * vx + vy * vy);

        let target00 = 1;
        let target11 = 1;
        if (speed > 5) {
            const stretch = 1 + Math.min(speed * deformScale, 0.35);
            const compress = 1 / stretch;
            if (Math.abs(vx) >= Math.abs(vy)) {
                target00 = stretch;
                target11 = compress;
            } else {
                target00 = compress;
                target11 = stretch;
            }
        }

        const invDamp = 1 / (1 + damping * dt);
        v00 = (v00 - stiffness * (m00 - target00) * dt) * invDamp;
        m00 += v00 * dt;
        v11 = (v11 - stiffness * (m11 - target11) * dt) * invDamp;
        m11 += v11 * dt;

        if (Math.abs(m00 - 1) + Math.abs(m11 - 1) < 0.004
                && Math.abs(v00) + Math.abs(v11) < 0.05 && speed < 5) {
            m00 = 1;
            m11 = 1;
            v00 = 0;
            v11 = 0;
            frameAnim.running = false;
        }
    }
}
