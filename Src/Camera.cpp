#include "Camera.h"
#include "FrameInfo.h"

namespace Eng {
    CameraAbstract::CameraAbstract() {

    }
    CameraAbstract::~CameraAbstract() {
        
    }
    void CameraAbstract::setOrtho(const float& left, const float& right, const float& bottom, const float& top, const float& near, const float& far) {
        projection = mat4(1.0f);
        projection[0][0] = 2.f / (right - left);
        projection[1][1] = 2.f / (bottom - top);
        projection[2][2] = 1.f / (far - near);
        projection[3][0] = -(right + left) / (right - left);
        projection[3][1] = -(bottom + top) / (bottom - top);
        projection[3][2] = -near / (far - near);
    }
    void CameraAbstract::setProj(const float& fovY, const float& aspect, const float& near, const float& far) {
        assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
        const float tanHalfFovY = glm::tan(fovY / 2.f);
        projection = mat4(0.0f);
        projection[0][0] = 1.f / (aspect * tanHalfFovY);
        projection[1][1] = 1.f / (tanHalfFovY);
        projection[2][2] = far / (far - near);
        projection[2][3] = 1.f;
        projection[3][2] = -(far * near) / (far - near);
    }
    
    void CameraAbstract::setViewDirection(const vec3& position, const vec3& direction, const vec3& up) {
        assert((glm::dot(direction, direction) > std::numeric_limits<float>::epsilon()) && "CameraAbstract look direction must not be zero.");
        const glm::vec3 w{glm::normalize(direction)};
        const glm::vec3 u{glm::normalize(glm::cross(w, up))};
        const glm::vec3 v{glm::cross(w, u)};

        view = mat4(
            vec4(u.x, v.x, w.x, 0.0f),// inverse, or transpose (for rotation matrices this is the same)
            vec4(u.y, v.y, w.y, 0.0f),
            vec4(u.z, v.z, w.z, 0.0f),
            vec4(-glm::dot(u, position), -glm::dot(v, position), -glm::dot(w, position), 1.0f)
        );
        inverseView = mat4(
            vec4(u.x, u.y, u.z, 0.0f),
            vec4(v.x, v.y, v.z, 0.0f),
            vec4(w.x, w.y, w.z, 0.0f),
            vec4(position.x, position.y, position.z, 1.0f)
        );
    }
    void CameraAbstract::setViewTarget(const vec3& position, const vec3& target, const vec3& up) {
        setViewDirection(position, target-position, up);
    }
    void CameraAbstract::setViewYXZ(const vec3& position, const vec3& rotation) {
        float s1 = glm::sin(rotation.y);
        float s2 = glm::sin(rotation.x);
        float s3 = glm::sin(rotation.z);
        float c1 = glm::cos(rotation.y);
        float c2 = glm::cos(rotation.x);
        float c3 = glm::cos(rotation.z);
        vec3 u = vec3(c1*c3+s1*s2*s3, c2*s3, c1*s2*s3-s1*c3);
        vec3 v = vec3(s1*s2*c3-c1*s3, c2*c3, c1*s2*c3+s1*s3);
        vec3 w = vec3(s1*c2, -s2, c1*c2);
        view = mat4(
            vec4(u.x, v.x, w.x, 0.0f),// inverse, or transpose (for rotation matrices this is the same)
            vec4(u.y, v.y, w.y, 0.0f),
            vec4(u.z, v.z, w.z, 0.0f),
            vec4(-glm::dot(u, position), -glm::dot(v, position), -glm::dot(w, position), 1.0f)
        );
        inverseView = mat4(
            vec4(u.x, u.y, u.z, 0.0f),
            vec4(v.x, v.y, v.z, 0.0f),
            vec4(w.x, w.y, w.z, 0.0f),
            vec4(position.x, position.y, position.z, 1.0f)
        );
    }

    Camera3D::Camera3D(const float& aspectRatio, const vec3& position, const vec3& rotation) {
        transform.position = position;
        transform.rotation = rotation;
        setViewYXZ(transform.position, transform.rotation);
        setProj(glm::radians(50.0f), aspectRatio, 0.1f, 100.0f);// must be done since aspect ratio can change.
    }
    Camera3D::~Camera3D() {
        
    }
    void Camera3D::pollMovement(const FrameInfo& frameInfo, const KeyMappings& keys) {
        float dt = glm::min(frameInfo.dt, 1.0f/30.0f);
        if (frameInfo.getKeyPressed(keys.pause)) {
            paused = !paused;
            if (paused) frameInfo.showCursor();
            else frameInfo.hideCursor();
        }
        if (paused) return;
        bool updated = false;
        vec2 dMouse = frameInfo.getMouseChange();
        if (glm::dot(dMouse, dMouse) > std::numeric_limits<float>::epsilon()){
            dvec2 temp = normalize(dMouse)*dt;
            transform.rotation += vec3(sensitivity.y*temp.y, sensitivity.x*temp.x, 0.0f);
            transform.rotation.x = glm::clamp(transform.rotation.x, -DEG90, DEG90);
            transform.rotation.y = glm::mod(transform.rotation.y, DEG360);
            updated = true;
        }
        const vec3 forward = vec3(glm::sin(transform.rotation.y), 0.0f, glm::cos(transform.rotation.y));
        const vec3 right = vec3(forward.z, 0.0f, -forward.x);// alternatively glm::cross(forward, up)
        vec3 movement(0.0f, 0.0f, 0.0f);
        if (frameInfo.getKeyHeld(keys.moveForward)) movement += forward;
        if (frameInfo.getKeyHeld(keys.moveBackward)) movement -= forward;
        if (frameInfo.getKeyHeld(keys.moveRight)) movement += right;
        if (frameInfo.getKeyHeld(keys.moveLeft)) movement -= right;
        if (frameInfo.getKeyHeld(keys.moveUp)) movement.y -= 1;
        if (frameInfo.getKeyHeld(keys.moveDown)) movement.y += 1;
        if (glm::dot(movement, movement) > std::numeric_limits<float>::epsilon()) {
            transform.position += speed*dt*normalize(movement); updated = true;
        }
        if (updated) setViewYXZ(transform.position, transform.rotation);
    }
}