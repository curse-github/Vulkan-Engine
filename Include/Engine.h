#ifndef ENG_ENGINE
#define ENG_ENGINE

#include "Helpers.h"
#include "Window.h"
#include "Device.h"
#include "Pipeline.h"
#include "RenderSystem.h"
#include "Renderers.h"
#include "Buffer.h"
#include "Descriptors.h"
#include "FrameInfo.h"
#include "UboStructs.h"
#include "ResourceManager.h"
#include "Camera.h"

namespace Eng {
    class Engine {
        KeyMappings keys{};
        bool started = false;

        Window window;
        Device device;
        EntitySystem entitySystem;
        
        OwnedPointer<DescriptorPool> globalDescriptorPool;
        unsigned int maxTextures;
        OwnedPointer<ResourceManager> resourceManager;

        typedef void (* UpdateCallbackT)(FrameInfo&);
        UpdateCallbackT updateCallback = nullptr;
    public:
        Engine(const std::string& windowName, const ivec2& windowSize);
        Engine(const Engine& copy) = delete;
        Engine& operator=(const Engine& copy) = delete;
        Engine(Engine&& move) = delete;
        Engine& operator=(Engine&& move) = delete;
        ~Engine();

        ECS_id_t addMeshRendereredEntity(
            const vec3& position, const vec3& scale, const vec3& rotation, const std::string& mesh,
            const std::string& mtlFile, const std::string& materialName, const float& normMult = 1.0f
        );
        ECS_id_t addLightEntity(const vec3& position, const float& size, const vec3& color, const float& intensity);
        
        void setUpdate(UpdateCallbackT _updateCallback);
        void start();
        void run();
    };
}

#endif