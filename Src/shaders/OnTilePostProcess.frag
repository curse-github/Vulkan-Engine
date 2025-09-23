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
layout (input_attachment_index = 0, set = 1, binding = 0) uniform subpassInput colorInput;
layout (input_attachment_index = 0, set = 1, binding = 1) uniform subpassInput depthInput;
layout (location = 0) in vec2 fragUv;

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
float LinearizeDepth_Vulkan(float d, float n, float f) {
    // For Vulkan (NDC z in [0,1])
    return (n * f) / (f - d * (f - n));
}
float getDepth(subpassInput depthInput) {
    return LinearizeDepth_Vulkan(subpassLoad(depthInput).r, 0.1, 100.0);
}

layout (location = 0) out vec4 outColor;

const float density = -0.02772588722;
void main() {
    vec3 color = subpassLoad(colorInput).rgb;
    float factor = exp(density*getDepth(depthInput));
    outColor = vec4(color*density, 1.0);
    // outColor = vec4(floor(color.rgb*10)/10, 1.0);
}