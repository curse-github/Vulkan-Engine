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
    public:
        DiffuseBlinnPhongRenderer(Device* _device,
            VkDescriptorSetLayout& globalDescriptorSetLayout, VkDescriptorSetLayout& materialDescriptorSetLayout, const unsigned int& numTextures, const unsigned int& numMaterials, DescriptorPool* globalDescriptorPool);
        DiffuseBlinnPhongRenderer(const DiffuseBlinnPhongRenderer& copy) = delete;
        DiffuseBlinnPhongRenderer& operator=(const DiffuseBlinnPhongRenderer& copy) = delete;
        DiffuseBlinnPhongRenderer(DiffuseBlinnPhongRenderer&& move) = delete;
        DiffuseBlinnPhongRenderer& operator=(DiffuseBlinnPhongRenderer&& move) = delete;
        virtual ~DiffuseBlinnPhongRenderer() = default;
        
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
        PointLightRenderer(Device* _device, VkDescriptorSetLayout& globalDescriptorSetLayout);
        PointLightRenderer(const PointLightRenderer& copy) = delete;
        PointLightRenderer& operator=(const PointLightRenderer& copy) = delete;
        PointLightRenderer(PointLightRenderer&& move) = delete;
        PointLightRenderer& operator=(PointLightRenderer&& move) = delete;
        virtual ~PointLightRenderer() = default;
        
        void render(FrameInfo& frameInfo);
    };

    class PostProcessRenderer : public RendererAbstract {
    public:
        PostProcessRenderer(Device* _device);
        PostProcessRenderer(const PostProcessRenderer& copy) = delete;
        PostProcessRenderer& operator=(const PostProcessRenderer& copy) = delete;
        PostProcessRenderer(PostProcessRenderer&& move) = delete;
        PostProcessRenderer& operator=(PostProcessRenderer&& move) = delete;
        virtual ~PostProcessRenderer() = default;
        
        void render(FrameInfo& frameInfo);
    };
}

#endif// ENG_RENDERSYSTEMS