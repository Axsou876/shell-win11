import QtQuick

NumberAnimation {
    enum Type {
        StandardSmall = 0,
        Standard,
        StandardLarge,
        StandardExtraLarge,
        EmphasizedSmall,
        Emphasized,
        EmphasizedLarge,
        EmphasizedExtraLarge,
        FastSpatial,
        DefaultSpatial,
        SlowSpatial,
        FastEffects,
        DefaultEffects,
        SlowEffects
    }

    property int type: Anim.DefaultSpatial

    // Values copied from Caelestia Config/tokens.hpp.
    readonly property var emphasized: [0.05, 0, 2.0 / 15.0, 0.06, 1.0 / 6.0, 0.4, 5.0 / 24.0, 0.82, 0.25, 1, 1, 1]
    readonly property var standard: [0.2, 0, 0, 1, 1, 1]
    readonly property var fastSpatial: [0.42, 1.67, 0.21, 0.9, 1, 1]
    readonly property var defaultSpatial: [0.38, 1.21, 0.22, 1, 1, 1]
    readonly property var slowSpatial: [0.39, 1.29, 0.35, 0.98, 1, 1]
    readonly property var fastEffects: [0.31, 0.94, 0.34, 1, 1, 1]
    readonly property var defaultEffects: [0.34, 0.8, 0.34, 1, 1, 1]
    readonly property var slowEffects: [0.34, 0.88, 0.34, 1, 1, 1]

    function durationFor(t) {
        if (t === Anim.FastSpatial)
            return 350;
        if (t === Anim.DefaultSpatial)
            return 500;
        if (t === Anim.SlowSpatial)
            return 650;
        if (t === Anim.FastEffects)
            return 150;
        if (t === Anim.DefaultEffects)
            return 200;
        if (t === Anim.SlowEffects)
            return 300;
        const standardDurations = [200, 400, 600, 1000];
        return standardDurations[t % 4];
    }

    function curveFor(t) {
        if (t === Anim.FastSpatial)
            return fastSpatial;
        if (t === Anim.DefaultSpatial)
            return defaultSpatial;
        if (t === Anim.SlowSpatial)
            return slowSpatial;
        if (t === Anim.FastEffects)
            return fastEffects;
        if (t === Anim.DefaultEffects)
            return defaultEffects;
        if (t === Anim.SlowEffects)
            return slowEffects;
        if (t >= Anim.EmphasizedSmall && t <= Anim.EmphasizedExtraLarge)
            return emphasized;
        return standard;
    }

    duration: durationFor(type)
    easing.type: Easing.BezierSpline
    easing.bezierCurve: curveFor(type)
}

