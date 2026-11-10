#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec4 sizeParams; // width, height, hole radius, smoothing
    vec4 frame; // hole x, y, width, height
    vec4 colour;
    vec4 panel0;
    vec4 radii0;
    vec4 deform0;
    vec4 panel1;
    vec4 radii1;
    vec4 deform1;
    vec4 panel2;
    vec4 radii2;
    vec4 deform2;
};

float sdBox(vec2 p, vec2 c, vec2 h) {
    vec2 d = abs(p - c) - h;
    return length(max(d, vec2(0.0))) + min(max(d.x, d.y), 0.0);
}

// r = (topRight, bottomRight, bottomLeft, topLeft)
float sdRoundedBox4(vec2 p, vec2 c, vec2 h, vec4 r) {
    p -= c;
    r.xy = p.x > 0.0 ? r.xy : r.wz;
    r.x = p.y > 0.0 ? r.y : r.x;
    vec2 q = abs(p) - h + r.x;
    return min(max(q.x, q.y), 0.0) + length(max(q, vec2(0.0))) - r.x;
}

// Circular smooth union, matching Caelestia's fillet geometry.
float smin(float a, float b, float k) {
    return max(k, min(a, b)) - length(max(vec2(k) - vec2(a, b), vec2(0.0)));
}

float panelSdf(vec2 p, vec4 rect, vec4 radii, vec4 deform) {
    vec2 centre = rect.xy + rect.zw * 0.5;
    mat2 inverseDeform = mat2(deform.x, deform.y, deform.z, deform.w);
    vec2 transformed = centre + inverseDeform * (p - centre);
    return sdRoundedBox4(transformed, centre, rect.zw * 0.5, radii);
}

// The original BlobInvertedRect recedes its inner wall where a panel enters
// from a screen border, forming the characteristic pocket. This is only
// active while a panel is still outside the hole; at rest the smooth union
// provides the permanent fillet.
float borderSink(vec2 p, vec4 panel, vec2 size, vec4 hole, float smoothing) {
    vec2 centre = panel.xy + panel.zw * 0.5;
    vec2 halfSize = panel.zw * 0.5;
    const float preOff = 0.29289321881 * smoothing; // (2 - sqrt(2)) / 2 * k

    float innerLeft = hole.x;
    float innerTop = hole.y;
    float innerRight = hole.x + hole.z;
    float innerBottom = hole.y + hole.w;
    float leftBorder = hole.x;
    float topBorder = hole.y;
    float rightBorder = size.x - innerRight;
    float bottomBorder = size.y - innerBottom;

    float hLat = max(abs(p.x - centre.x) - halfSize.x, 0.0);
    float vLat = max(abs(p.y - centre.y) - halfSize.y, 0.0);
    float hFall = 1.0 - smoothstep(0.0, smoothing * 2.0, hLat);
    float vFall = 1.0 - smoothstep(0.0, smoothing * 2.0, vLat);
    float s = 0.0;

    float topPen = clamp(innerTop - (panel.y + panel.w) - preOff, 0.0, topBorder);
    if (topPen > 0.0)
        s = max(s, topPen * hFall * (1.0 - smoothstep(innerTop, innerTop + smoothing, p.y)));

    float bottomPen = clamp(panel.y - innerBottom - preOff, 0.0, bottomBorder);
    if (bottomPen > 0.0)
        s = max(s, bottomPen * hFall
                * smoothstep(innerBottom - smoothing, innerBottom, p.y));

    float leftPen = clamp(innerLeft - (panel.x + panel.z) - preOff, 0.0, leftBorder);
    if (leftPen > 0.0)
        s = max(s, leftPen * vFall * (1.0 - smoothstep(innerLeft, innerLeft + smoothing, p.x)));

    float rightPen = clamp(panel.x - innerRight - preOff, 0.0, rightBorder);
    if (rightPen > 0.0)
        s = max(s, rightPen * vFall * smoothstep(innerRight - smoothing, innerRight, p.x));

    return s;
}

void main() {
    vec2 p = qt_TexCoord0 * sizeParams.xy;
    float smoothing = sizeParams.w;

    // The surface is the work area minus a rounded hole. The 60 px left band
    // is the bar; the top/right/bottom bands are the 10 px frame.
    vec2 halfSize = sizeParams.xy * 0.5;
    vec4 hole = frame;
    vec2 holeCentre = hole.xy + hole.zw * 0.5;

    float sink = 0.0;
    if (panel0.z > 0.5 && panel0.w > 0.5)
        sink = max(sink, borderSink(p, panel0, sizeParams.xy, hole, smoothing));
    if (panel1.z > 0.5 && panel1.w > 0.5)
        sink = max(sink, borderSink(p, panel1, sizeParams.xy, hole, smoothing));
    if (panel2.z > 0.5 && panel2.w > 0.5)
        sink = max(sink, borderSink(p, panel2, sizeParams.xy, hole, smoothing));

    float dOuter = sdBox(p, halfSize, halfSize);
    float dHole = sdRoundedBox4(p, holeCentre, hole.zw * 0.5, vec4(sizeParams.z)) - sink;
    float d = max(dOuter, -dHole);

    if (panel0.z > 0.5 && panel0.w > 0.5)
        d = smin(d, panelSdf(p, panel0, radii0, deform0), smoothing);
    if (panel1.z > 0.5 && panel1.w > 0.5)
        d = smin(d, panelSdf(p, panel1, radii1, deform1), smoothing);
    if (panel2.z > 0.5 && panel2.w > 0.5)
        d = smin(d, panelSdf(p, panel2, radii2, deform2), smoothing);

    float fw = max(fwidth(d), 0.0001);
    float alpha = (1.0 - smoothstep(-fw, fw, d)) * colour.a;
    if (alpha <= 0.0)
        discard;

    fragColor = vec4(colour.rgb * alpha, alpha) * qt_Opacity;
}
