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

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput sceneInput;
layout (location = 0) in vec2 fragUv;

layout (location = 0) out vec4 outColor;

void main() {
    vec4 color = subpassLoad(sceneInput);
    outColor = vec4(floor(color.rgb*5)/5, 1.0);
}