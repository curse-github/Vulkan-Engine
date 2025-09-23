#version 450

const vec2 Positions[3] = vec2[](
    vec2(-2.0,-1.0  ),
    vec2( 0.0, 1.732),
    vec2( 2.0,-1.0  )
);

layout (location = 0) out vec3 vertPosition;

layout(push_constant) uniform Push {
    vec4 positionSize;
    vec4 colorIntensity;
};

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

void main() {
    vec3 right = vec3(inverseView[0][0], inverseView[0][1], inverseView[0][2]);
    vec3 up = vec3(inverseView[1][0], inverseView[1][1], inverseView[1][2]);
    vertPosition = vec3(Positions[gl_VertexIndex], 0.0f);
    vec3 worldPosition = positionSize.xyz + positionSize.w * (vertPosition.x*right + vertPosition.y*up);
    gl_Position = projectionView * vec4(worldPosition, 1.0);
}