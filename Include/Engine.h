#ifndef ENG_ENGINE
#define ENG_ENGINE

#include "Helpers.h"
#include "Window.h"
#include "Device.h"
#include "Pipeline.h"
#include "RenderSystem.h"
#include "Mesh.h"
#include "Renderers.h"
#include "Loaders.h"
#include "Buffer.h"
#include "Descriptors.h"
#include "FrameInfo.h"
#include "UboStructs.h"
#include "Texture.h"

namespace Eng {
    class Engine {
        KeyMappings keys{};
        float speed = 3.0f;
        vec2 sensitivity = {5.0f, -4.0f};

        Window window;
        Device device;
        EntitySystem entitySystem;
        
        OwnedPointer<DescriptorPool> globalDescriptorPool;
        std::unordered_map<std::string, OwnedPointer<Mesh>> meshes;
        unsigned int maxTextures;
        std::vector<OwnedPointer<Texture>> textures;
        std::unordered_map<std::string, size_t> textureIdxs;
        std::unordered_map<std::string, unsigned int> loadedMtls;
        std::vector<MaterialUboData> materials;
        std::unordered_map<std::string, size_t> materialIdxs;

        typedef void (* UpdateCallbackT)(FrameInfo&);
        UpdateCallbackT updateCallback = nullptr;

        bool started = false;
        bool paused = false;
        bool pollMovement(const float& dt, TransformComponent& transform);
    public:
        Engine(const std::string& windowName, const ivec2& windowSize);
        Engine(const Engine& copy) = delete;
        Engine& operator=(const Engine& copy) = delete;
        Engine(Engine&& move) = delete;
        Engine& operator=(Engine&& move) = delete;
        ~Engine();

        unsigned int storeTexture(const std::string& texture);
        unsigned int storeMaterial(const std::string& materialName, const MaterialUboData& data);
        ECS_id_t addMeshRendereredEntity(const vec3& position, const vec3& scale, const vec3& rotation, const std::string& mesh, const std::string& materialFile, const std::string& material, const float& normMult);
        ECS_id_t addLightEntity(const vec3& position, const float& size, const vec3& color, const float& intensity);
        
        void setUpdate(UpdateCallbackT _updateCallback);
        void start();
        void run();
    };
}

#endif