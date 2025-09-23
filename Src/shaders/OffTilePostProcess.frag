#version 450

vec3 toneMap(vec3 x) {
    // ACES tone mapping
    // Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

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
layout (set = 1, binding = 0) uniform sampler2D screenTex;
layout (location = 1) in vec2 fragPixelPos;

layout (location = 0) out vec4 outColor;
void main() {
    vec3 color = vec3(0.0, 0.0, 0.0);
    for(int i = -3; i <= 3; i++)
        for(int j = -3; j <= 3; j++)
            color += textureLod(screenTex, fragPixelPos + vec2(i, j), 0.0).rgb;
    outColor = vec4(color/49.0, 1.0);
}