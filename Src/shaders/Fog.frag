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

float LinearizeDepth_Vulkan(float d, float n, float f) {
    // For Vulkan (NDC z in [0,1])
    return (n * f) / (f - d * (f - n));
}
float getDepth(subpassInput depthInput) {
    return LinearizeDepth_Vulkan(subpassLoad(depthInput).r, 0.1, 100.0);
}

const float density = -0.09902102579;

layout (location = 0) out vec4 outColor;
void main() {
    vec3 color = subpassLoad(colorInput).rgb;
    float factor = exp(density*getDepth(depthInput));
    outColor = vec4(color*factor, 1.0);
}