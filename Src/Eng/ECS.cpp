#include "ECS.h"
namespace Eng {
    Entity::~Entity() {
        system->RemoveAll(id);
    }
    mat4 TransformComponent::getTransformMat() const {
        float s1 = glm::sin(rotation.y);
        float s2 = glm::sin(rotation.x);
        float s3 = glm::sin(rotation.z);
        float c1 = glm::cos(rotation.y);
        float c2 = glm::cos(rotation.x);
        float c3 = glm::cos(rotation.z);
        return mat4(
            scale.x*vec4(c1*c3+s1*s2*s3, c2*s3, c1*s2*s3-s1*c3, 0.0f),
            scale.y*vec4(s1*s2*c3-c1*s3, c2*c3, c1*s2*c3+s1*s3, 0.0f),
            scale.z*vec4(s1*c2, -s2, c1*c2, 0.0f),
            vec4(position, 1.0f)
        );
    }
    mat3 TransformComponent::getNormalMat() const {
        float s1 = glm::sin(rotation.y);
        float s2 = glm::sin(rotation.x);
        float s3 = glm::sin(rotation.z);
        float c1 = glm::cos(rotation.y);
        float c2 = glm::cos(rotation.x);
        float c3 = glm::cos(rotation.z);
        return mat3(
            vec3(c1*c3+s1*s2*s3, c2*s3, c1*s2*s3-s1*c3)/scale.x,
            vec3(s1*s2*c3-c1*s3, c2*c3, c1*s2*c3+s1*s3)/scale.y,
            vec3(s1*c2,-s2,c1*c2)/scale.z
        );
    }
}