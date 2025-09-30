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
        OwnedPointer<ResourceManager> resourceManager;
        
        unsigned int maxTextures;

        typedef void (* UpdateCallbackT)(FrameInfo&);
        UpdateCallbackT updateCallback = nullptr;
    public:
        Engine(const std::string& windowName, const ivec2& windowSize);
        Engine(const Engine& copy) = delete;
        Engine& operator=(const Engine& copy) = delete;
        Engine(Engine&& move) = delete;
        Engine& operator=(Engine&& move) = delete;
        ~Engine();

        void loadMtlFile(const std::string& mtlFile);
        ECS_id_t addMeshRendereredEntity(
            const vec3& position, const vec3& scale, const vec3& rotation,
            const std::string& mesh, const std::string& materialName
        );
        ECS_id_t addLightEntity(const vec3& position, const float& size, const vec3& color, const float& intensity);
        
        void setUpdate(UpdateCallbackT _updateCallback);
        void start();
        void run();
    };
}

#endif