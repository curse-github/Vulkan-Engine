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
float LinearizeDepth_Vulkan(float d, float n, float f) {
    // For Vulkan (NDC z in [0,1])
    return (n * f) / (f - d * (f - n));
}

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput colorInput;
layout (location = 0) in vec2 fragUv;

layout (location = 0) out vec4 outColor;

void main() {
    vec3 color = subpassLoad(colorInput).rgb;
    outColor = vec4(color, 1.0);
}