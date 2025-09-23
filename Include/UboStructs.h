#ifndef __UBOSTRUCTS
#define __UBOSTRUCTS

#include "Helpers.h"

namespace Eng {
    #define MAX_LIGHTS 15u
    struct pointLight {
        vec4 positionSize{0.0f};
        vec4 colorIntensity{0.0f};
    };
    struct GlobalUboData {
        mat4 projectionView{1.0f};
        mat4 inverseView{1.0f};
        vec4 ambientLightColor{1.0f, 1.0f, 1.0f, 0.2f};
        vec4 directionLightDirection{1.414f, -1.414f, 0.0f, 0.0f};
        vec4 directionLightColorIntensity{1.0f, 1.0f, 1.0f, 1.0f};
        vec4 resolution{5.0f, 5.0f, 0.0f, 0.0f};
        unsigned int numLights{0};
        alignas(16) pointLight pointLights[MAX_LIGHTS];
    };
    struct MaterialUboData {
        vec4 diffuseColor_Transparency{1.0f, 1.0f, 1.0f, 1.0f};
        vec4 specColor_Exp{1.0f, 1.0f, 1.0f, 0.0f};
        unsigned int map_diff = 0;
        unsigned int map_specC = 0;
        unsigned int map_specE = 0;
        unsigned int map_norm = 1;
        vec4 normMult{0.0f, 0.0f, 1.0f, 1.0f};
        // tangent found from uv
        // bi_tange = cross(normal, tangent)
        // normal = map_norm.r * tangent
        //          + map_norm.g * bi_tangent
        //          + map_norm.b * normal
    };
}

#endif