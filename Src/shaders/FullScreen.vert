#version 450

const vec2 Positions[6] = vec2[](
    vec2(- 1.0, - 1.0),// bottom left
    vec2(- 1.0, 1.0),// top left
    vec2( 1.0, 1.0),// top right
    vec2(- 1.0, - 1.0),// bottom left
    vec2( 1.0, 1.0),// top right
    vec2( 1.0, - 1.0) // bottom right
);

layout (constant_id = 0) const int MAX_LIGHTS = 1;
struct Light {
    vec4 position;
    vec4 colorIntensity;
};
layout(set = 0, binding = 0) uniform GlobalUboData {
    mat4 projectionView;
    mat4 inverseView;
    vec4 ambientLightColor;
    vec4 directionLightDirection;
    vec4 directionLightColorIntensity;
    vec4 resolution;
    uint numLights;
    Light lights[MAX_LIGHTS];
};

layout (location = 0) out vec2 vertUv;
layout (location = 1) out vec2 vertPixelPos;

void main() {
    vec2 pos = Positions[gl_VertexIndex];
    gl_Position = vec4(pos, 0.0, 1.0);
    vertUv = vec2(0.5, 0.5) + pos*0.5;
    vertPixelPos = vertUv*resolution.xy;
}