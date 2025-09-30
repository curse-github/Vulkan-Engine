#ifndef ENG_RENDERSYSTEMS
#define ENG_RENDERSYSTEMS

#include "Helpers.h"
#include "Device.h"
#include "Pipeline.h"
#include "FrameInfo.h"
#include "UboStructs.h"
#include "Descriptors.h"
#include "Swapchain.h"
#include "RenderSystem.h"

namespace Eng {
    struct MeshRendererComponent : Component {
        std::string mesh;
        std::string material;
        MeshRendererComponent() = default;
        MeshRendererComponent(const MeshRendererComponent& copy) = delete;
        MeshRendererComponent& operator=(const MeshRendererComponent& copy) = delete;
        MeshRendererComponent(MeshRendererComponent&& move) = delete;
        MeshRendererComponent& operator=(MeshRendererComponent&& move) = delete;
        virtual ~MeshRendererComponent() = default;
    };
    class MeshRenderer : public RendererAbstract {
        ResourceManager::MappedUniformData* materialIndexUniform = nullptr;
    public:
        MeshRenderer(Device* _device, VkDescriptorSetLayout _globalDescriptorSetLayout, ResourceManager* resourceManager);
        MeshRenderer(const MeshRenderer& copy) = delete;
        MeshRenderer& operator=(const MeshRenderer& copy) = delete;
        MeshRenderer(MeshRenderer&& move) = delete;
        MeshRenderer& operator=(MeshRenderer&& move) = delete;
        virtual ~MeshRenderer() = default;
        void construct() override;
        
        void render(FrameInfo& frameInfo);
    };
    class DeferredGeometryPass : public RendererAbstract {
        ResourceManager::MappedUniformData* materialIndexUniform = nullptr;
    public:
        DeferredGeometryPass(Device* _device, VkDescriptorSetLayout _globalDescriptorSetLayout, ResourceManager* resourceManager);
        DeferredGeometryPass(const DeferredGeometryPass& copy) = delete;
        DeferredGeometryPass& operator=(const DeferredGeometryPass& copy) = delete;
        DeferredGeometryPass(DeferredGeometryPass&& move) = delete;
        DeferredGeometryPass& operator=(DeferredGeometryPass&& move) = delete;
        virtual ~DeferredGeometryPass() = default;
        void construct() override;
        
        void render(FrameInfo& frameInfo);
    };
    class DeferredRenderPass : public RendererAbstract {
    public:
        DeferredRenderPass(Device* _device, VkDescriptorSetLayout globalDescriptorSetLayout, ResourceManager* resourceManager);
        DeferredRenderPass(const DeferredRenderPass& copy) = delete;
        DeferredRenderPass& operator=(const DeferredRenderPass& copy) = delete;
        DeferredRenderPass(DeferredRenderPass&& move) = delete;
        DeferredRenderPass& operator=(DeferredRenderPass&& move) = delete;
        virtual ~DeferredRenderPass() = default;
        void construct() override;
        
        void render(FrameInfo& frameInfo);
    };
    
    struct PointLightComponent : Component{
        vec4 colorIntensity{0.0f};
        PointLightComponent() = default;
        PointLightComponent(const PointLightComponent& copy) = delete;
        PointLightComponent& operator=(const PointLightComponent& copy) = delete;
        PointLightComponent(PointLightComponent&& move) = delete;
        PointLightComponent& operator=(PointLightComponent&& move) = delete;
        virtual ~PointLightComponent() = default;
    };
    struct PointLightPushConstantData {
        vec4 positionSize{0.0f};
        vec4 colorIntensity{0.0f};
    };
    class PointLightRenderer : public RendererAbstract {
    public:
        PointLightRenderer(Device* _device, VkDescriptorSetLayout _globalDescriptorSetLayout, ResourceManager* resourceManager);
        PointLightRenderer(const PointLightRenderer& copy) = delete;
        PointLightRenderer& operator=(const PointLightRenderer& copy) = delete;
        PointLightRenderer(PointLightRenderer&& move) = delete;
        PointLightRenderer& operator=(PointLightRenderer&& move) = delete;
        virtual ~PointLightRenderer() = default;
        
        void render(FrameInfo& frameInfo);
    };

    class OnTilePostProcessRenderer : public RendererAbstract {
    public:
        OnTilePostProcessRenderer(Device* _device, VkDescriptorSetLayout globalDescriptorSetLayout, ResourceManager* resourceManager, const std::string& pixelShader);
        OnTilePostProcessRenderer(const OnTilePostProcessRenderer& copy) = delete;
        OnTilePostProcessRenderer& operator=(const OnTilePostProcessRenderer& copy) = delete;
        OnTilePostProcessRenderer(OnTilePostProcessRenderer&& move) = delete;
        OnTilePostProcessRenderer& operator=(OnTilePostProcessRenderer&& move) = delete;
        virtual ~OnTilePostProcessRenderer() = default;
        
        void render(FrameInfo& frameInfo);
    };

    class OffTilePostProcessRenderer : public RendererAbstract {
    public:
        OffTilePostProcessRenderer(Device* _device, VkDescriptorSetLayout globalDescriptorSetLayout, ResourceManager* resourceManager, const std::string& pixelShader);
        OffTilePostProcessRenderer(const OffTilePostProcessRenderer& copy) = delete;
        OffTilePostProcessRenderer& operator=(const OffTilePostProcessRenderer& copy) = delete;
        OffTilePostProcessRenderer(OffTilePostProcessRenderer&& move) = delete;
        OffTilePostProcessRenderer& operator=(OffTilePostProcessRenderer&& move) = delete;
        virtual ~OffTilePostProcessRenderer() = default;
        
        void render(FrameInfo& frameInfo);
    };
}

#endif// ENG_RENDERSYSTEMS