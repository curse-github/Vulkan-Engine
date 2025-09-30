#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout (input_attachment_index = 0, set = 2, binding = 0) uniform subpassInput inWorldPosU;
layout (input_attachment_index = 1, set = 2, binding = 1) uniform subpassInput inVNormal;
layout (input_attachment_index = 2, set = 2, binding = 2) uniform subpassInput inTangent;
layout (input_attachment_index = 3, set = 2, binding = 3) uniform subpassInput inMatIdx;

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

layout (constant_id = 1) const int NUM_TEXTURES = 1;
layout(set = 1, binding = 0) uniform sampler2D texSamplers[NUM_TEXTURES];

layout (constant_id = 2) const int NUM_MATERIALS = 1;
struct Material {
    vec4 diffuseColor_Transparency;
    vec4 specColor_Exp;
    uint map_diff;
    uint map_specC;
    uint map_specE;
    uint map_norm;
    float normMult;
};
layout(set = 1, binding = 1) uniform Materials { Material materials[NUM_MATERIALS]; };

layout (location = 0) out vec4 outColor;
void main() {
    vec4 WorldPosU = subpassLoad(inWorldPosU);
    vec3 fragWorldPosition = WorldPosU.xyz;
    vec4 VNormal = subpassLoad(inVNormal);
    vec2 fragUv = vec2(WorldPosU.w, VNormal.x);
    vec3 fragNormal = VNormal.yzw;
    vec4 fragTangent = subpassLoad(inTangent);
    uint matIdx = uint(subpassLoad(inMatIdx).x);
    
    Material mat = materials[nonuniformEXT(matIdx)];
    vec4 diffuseColor = mat.diffuseColor_Transparency*texture(texSamplers[nonuniformEXT(mat.map_diff)], fragUv);
    // initialize some stuff for lights
    vec3 ogNormal = normalize(fragNormal);
    vec3 tangent = normalize(fragTangent.xyz);
    mat3 tangentSpace = mat3(
        tangent,
        cross(ogNormal, tangent)*fragTangent.w,
        ogNormal
    );
    vec3 tangentSpaceNormal = normalize(((texture(texSamplers[nonuniformEXT(mat.map_norm)], fragUv*mat.normMult).xyz)*2-vec3(1.0)));
    tangentSpaceNormal += vec3(0.0, 0.0, float(dot(tangentSpaceNormal, tangentSpaceNormal)==0));
    vec3 normal = tangentSpace*tangentSpaceNormal;
    vec3 diffuseLightColor = ambientLightColor.xyz*ambientLightColor.w;
    vec3 specularLightColor = vec3(0.0);
    vec3 viewDirection = normalize(inverseView[3].xyz-fragWorldPosition);
    vec3 specColor = mat.specColor_Exp.xyz*texture(texSamplers[nonuniformEXT(mat.map_specC)], fragUv).xyz;
    float N = (mat.specColor_Exp.w+0.001)*texture(texSamplers[nonuniformEXT(mat.map_specE)], fragUv).r;
    float S = (N+8)/25.1327412287;
    // loop through lights
    {
        vec3 lightColor = directionLightColorIntensity.xyz*directionLightColorIntensity.w * max(dot(normal, normalize(directionLightDirection.xyz)), 0.0);
        // diffuse light
        diffuseLightColor += lightColor;
        // specular light
        float blinnPhongTerm = clamp(dot(normal, normalize(directionLightDirection.xyz + viewDirection)), 0, 1);// normal . halfAngle
        specularLightColor += lightColor*pow(blinnPhongTerm, N)*S;
    }
    for(uint i = 0; i < numLights; i++) {
        // lighting data
        Light light = lights[i];
        vec3 lightDirection = light.position.xyz - fragWorldPosition;
        float attenuation = max(dot(lightDirection, lightDirection), 0.25);
        lightDirection = normalize(lightDirection);
        vec3 lightColor = light.colorIntensity.xyz*light.colorIntensity.w * max(dot(normal, lightDirection), 0.0) / attenuation;
        // diffuse light
        diffuseLightColor += lightColor;
        // specular light
        float blinnPhongTerm = clamp(dot(normal, normalize(lightDirection + viewDirection)), 0, 1);// normal . halfAngle
        specularLightColor += lightColor*pow(blinnPhongTerm, N)*S;
    }
    outColor = vec4(diffuseLightColor*diffuseColor.rgb + specularLightColor*specColor, diffuseColor.a);
}