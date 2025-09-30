#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec3 fragWorldPosition;
layout (location = 1) in vec2 fragUv;
layout (location = 2) in vec3 fragNormal;
layout (location = 3) in vec4 fragTangent;

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

layout(set = 1, binding = 0) uniform Idxs { uint matIdx; };

layout (location = 0) out vec4 outWorldPosU;
layout (location = 1) out vec4 outVNormal;
layout (location = 2) out vec4 outTangent;
layout (location = 3) out vec4 outMaterial;
void main() {
    outWorldPosU = vec4(fragWorldPosition, fragUv.x);
    outVNormal = vec4(fragUv.y, fragNormal);
    outTangent = fragTangent;
    outMaterial = vec4(matIdx, 0.0, 0.0, 0.0);
}