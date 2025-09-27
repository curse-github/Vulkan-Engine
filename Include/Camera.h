#ifndef ENG_CAMERA
#define ENG_CAMERA

#include <glm/vec3.hpp>
using glm::vec3;
#include <glm/vec4.hpp>
using glm::vec4;
#include <glm/mat4x4.hpp>
using glm::mat4;
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include "Window.h"
#include "ECS.h"

namespace Eng {
    class FrameInfo;
    class CameraAbstract {
    public:
        CameraAbstract();
        CameraAbstract(const CameraAbstract& copy) = delete;
        CameraAbstract& operator=(const CameraAbstract& copy) = delete;
        CameraAbstract(CameraAbstract&& move) = delete;
        CameraAbstract& operator=(CameraAbstract&& move) = delete;
        virtual ~CameraAbstract();

        mat4 projection;
        mat4 view;
        mat4 inverseView;
        void setOrtho(const float& left, const float& right, const float& bottom, const float& top, const float& near, const float& far);
        void setProj(const float& fovY, const float& aspect, const float& near, const float& far);

        void setViewDirection(const vec3& position, const vec3& direction, const vec3& up={0.0f, -1.0f, 0.0f});
        void setViewTarget(const vec3& position, const vec3& target, const vec3& up={0.0f, -1.0f, 0.0f});
        void setViewYXZ(const vec3& position, const vec3& rotation);

        vec3 getPosition() const { return vec3(inverseView[3]); };
    };
    class Camera3D : public CameraAbstract {
        TransformComponent transform{};
    public:
        float speed = 3.0f;
        vec2 sensitivity = {5.0f, -4.0f};
        bool paused = false;
        Camera3D(const float& aspectRatio, const vec3& position = vec3(0.0f), const vec3& rotation = vec3(0.0f));
        Camera3D(const Camera3D& copy) = delete;
        Camera3D& operator=(const Camera3D& copy) = delete;
        Camera3D(Camera3D&& move) = delete;
        Camera3D& operator=(Camera3D&& move) = delete;
        virtual ~Camera3D();

        void pollMovement(const FrameInfo& frameInfo, const KeyMappings& keys);
    };
}

#endif// ENG_CAMERA