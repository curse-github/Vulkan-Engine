#ifndef ENG_RENDERSYSTEMS
#define ENG_RENDERSYSTEMS

#include "Helpers.h"
#include "Device.h"
#include "Pipeline.h"
#include "Mesh.h"
#include "FrameInfo.h"
#include "UboStructs.h"
#include "Descriptors.h"
#include "Swapchain.h"
#include "RenderSystem.h"
#include "ECS.h"

namespace Eng {

    struct MeshRendererComponent : Component {
        Mesh* mesh;
        unsigned int materialIdx;
        MeshRendererComponent() = default;
        MeshRendererComponent(const MeshRendererComponent& copy) = delete;
        MeshRendererComponent& operator=(const MeshRendererComponent& copy) = delete;
        MeshRendererComponent(MeshRendererComponent&& move) = delete;
        MeshRendererComponent& operator=(MeshRendererComponent&& move) = delete;
        virtual ~MeshRendererComponent() = default;
    };
    class DiffuseBlinnPhongRenderer : public RendererAbstract {
        OwnedPointer<DescriptorSetLayout> materialIndexDescriptorSetLayout;
        VkDescriptorSet materialIndexDescriptorSet;
        OwnedPointer<Buffer> materialIndexUniformBuffer;
        VkDescriptorSetLayout& globalDescriptorSetLayout;
        VkDescriptorSetLayout& materialDescriptorSetLayout;
        DescriptorPool* globalDescriptorPool;
    public:
        DiffuseBlinnPhongRenderer(Device* _device, const unsigned int& numTextures, const unsigned int& numMaterials,
            VkDescriptorSetLayout& _globalDescriptorSetLayout, VkDescriptorSetLayout& _materialDescriptorSetLayout, DescriptorPool* _globalDescriptorPool);
        DiffuseBlinnPhongRenderer(const DiffuseBlinnPhongRenderer& copy) = delete;
        DiffuseBlinnPhongRenderer& operator=(const DiffuseBlinnPhongRenderer& copy) = delete;
        DiffuseBlinnPhongRenderer(DiffuseBlinnPhongRenderer&& move) = delete;
        DiffuseBlinnPhongRenderer& operator=(DiffuseBlinnPhongRenderer&& move) = delete;
        virtual ~DiffuseBlinnPhongRenderer() = default;
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
        VkDescriptorSetLayout& globalDescriptorSetLayout;
    public:
        PointLightRenderer(Device* _device, VkDescriptorSetLayout& _globalDescriptorSetLayout);
        PointLightRenderer(const PointLightRenderer& copy) = delete;
        PointLightRenderer& operator=(const PointLightRenderer& copy) = delete;
        PointLightRenderer(PointLightRenderer&& move) = delete;
        PointLightRenderer& operator=(PointLightRenderer&& move) = delete;
        virtual ~PointLightRenderer() = default;
        void construct() override;
        
        void render(FrameInfo& frameInfo);
    };

    class OnTilePostProcessRenderer : public RendererAbstract {
        VkDescriptorSetLayout& globalDescriptorSetLayout;
    public:
        OnTilePostProcessRenderer(Device* _device, const std::string& pixelShader, VkDescriptorSetLayout& globalDescriptorSetLayout);
        OnTilePostProcessRenderer(const OnTilePostProcessRenderer& copy) = delete;
        OnTilePostProcessRenderer& operator=(const OnTilePostProcessRenderer& copy) = delete;
        OnTilePostProcessRenderer(OnTilePostProcessRenderer&& move) = delete;
        OnTilePostProcessRenderer& operator=(OnTilePostProcessRenderer&& move) = delete;
        virtual ~OnTilePostProcessRenderer() = default;
        void construct() override;
        
        void render(FrameInfo& frameInfo);
    };

    class OffTilePostProcessRenderer : public RendererAbstract {
        VkDescriptorSetLayout& globalDescriptorSetLayout;
    public:
        OffTilePostProcessRenderer(Device* _device, const std::string& pixelShader, VkDescriptorSetLayout& globalDescriptorSetLayout);
        OffTilePostProcessRenderer(const OffTilePostProcessRenderer& copy) = delete;
        OffTilePostProcessRenderer& operator=(const OffTilePostProcessRenderer& copy) = delete;
        OffTilePostProcessRenderer(OffTilePostProcessRenderer&& move) = delete;
        OffTilePostProcessRenderer& operator=(OffTilePostProcessRenderer&& move) = delete;
        virtual ~OffTilePostProcessRenderer() = default;
        void construct() override;
        
        void render(FrameInfo& frameInfo);
    };
}

#endif// ENG_RENDERSYSTEMS