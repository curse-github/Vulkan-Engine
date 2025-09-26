#version 450

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
layout (set = 1, binding = 0) uniform sampler2D colorTex;
layout (location = 1) in vec2 fragPixelPos;

layout (location = 0) out vec4 outColor;
void main() {
    vec3 color = textureLod(colorTex, fragPixelPos, 0.0).rgb;
    outColor = vec4(color, 1.0);
}