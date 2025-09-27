#ifndef __FRAMEINFO
#define __FRAMEINFO

#include "Helpers.h"
#include "ECS.h"
#include "Camera.h"

namespace Eng {
    struct FrameInfo {
    private:
        Window* window;
    public:
        unsigned int imageIndex = 0u;
        unsigned int frameIndex = 0u;
        float t = 0.0f;
        float dt;
        VkCommandBuffer commandBuffer;
        Camera3D* camera;
        VkDescriptorSet globalDescriptorSet;
        EntitySystem* entitySystem;

        FrameInfo(Window* _window, Camera3D* const & _camera, EntitySystem* const & _entitySystem)
            : window(_window), camera(_camera), entitySystem(_entitySystem) {}
        void updateTime() {
            std::chrono::_V2::system_clock::time_point newTime = std::chrono::high_resolution_clock::now();
            dt = std::chrono::duration<float, std::chrono::seconds::period>(newTime-lastTime).count();
            lastTime = newTime;
            t += dt;
            // calc fps
            frames++;
            float diff = std::chrono::duration<float, std::chrono::seconds::period>(newTime-lastPrint).count();
            if (diff >= 2.5f) {
                std::cout << "fps: " << (frames/diff) << '\n';
                frames=0; lastPrint = newTime;
            }
        }
        bool getKeyPressed(const int& key) const {
            return window->getKeyPressed(key);
        }
        bool getKeyHeld(const int& key) const {
            return window->getKeyHeld(key);
        }
        vec2 getMouseChange() const {
            return window->getMouseChange();
        }
        void hideCursor() const {
            return window->hideCursor();
        }
        void showCursor() const {
            return window->showCursor();
        }
    private:
        std::chrono::_V2::system_clock::time_point lastPrint = std::chrono::high_resolution_clock::now();
        std::chrono::_V2::system_clock::time_point lastTime = std::chrono::high_resolution_clock::now();
        unsigned int frames = 0;
    };
}

#endif// __FRAMEINFO