#ifndef ENG_GAMEOBJECT
#define ENG_GAMEOBJECT

#include "Helpers.h"
#include "Mesh.h"
#include "UboStructs.h"

namespace Eng {
    struct MaterialUboData;
    struct TransformComponent {
        vec3 position{0.0f, 0.0f, 0.0f};
        vec3 scale{1.0f, 1.0f, 1.0f};
        vec3 rotation{0.0f, 0.0f, 0.0f};
        mat4 getTransformMat() const;
        mat3 getNormalMat() const;
        TransformComponent() {};
        TransformComponent(const TransformComponent& copy) : position(copy.position), scale(copy.scale), rotation(copy.rotation) { }
        TransformComponent& operator=(const TransformComponent& copy) {
            position = copy.position;
            scale = copy.scale;
            rotation = copy.rotation;
            return *this;
        }
    };
    struct PointLightComponent {
        vec4 colorIntensity{0.0f};
        PointLightComponent(const vec4& _colorIntensity) : colorIntensity(_colorIntensity) { };
    };
    class GameObject {
    public:
        using id_t = unsigned int;
        using Map = std::unordered_map<id_t, GameObject>;
        GameObject();
        GameObject(const GameObject& copy) = delete;
        GameObject& operator=(const GameObject& copy) = delete;
        GameObject(GameObject&& move);
        GameObject& operator=(GameObject&& move) = delete;
        ~GameObject();

        id_t id;
        TransformComponent transform;
        Mesh* mesh;
        PointLightComponent* light = nullptr;
        unsigned int materialIdx;
    private:
        GameObject(const id_t& _id);
    };
}

#endif