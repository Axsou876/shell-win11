#version 440

layout(location = 0) in vec4 qt_VertexPosition;
layout(location = 1) in vec2 qt_VertexTexCoord;

layout(location = 0) out vec2 qt_TexCoord0;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec4 sizeParams; // width, height, hole radius, smoothing
    vec4 frame; // hole x, y, width, height
    vec4 colour;
    vec4 shadowColour;
    vec4 shadowParams;
    vec4 panel0;
    vec4 radii0;
    vec4 deform0;
    vec4 panel1;
    vec4 radii1;
    vec4 deform1;
    vec4 panel2;
    vec4 radii2;
    vec4 deform2;
    vec4 panel3;
    vec4 radii3;
    vec4 deform3;
};

void main() {
    gl_Position = qt_Matrix * qt_VertexPosition;
    qt_TexCoord0 = qt_VertexTexCoord;
}
