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
    class RendererAbstract {
    protected:
        Device* device;
        RenderSystem* renderSystem;
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        PipelineConfigInfo pipelineConfig{};
        VkPipelineLayout pipelineLayout;
        Pipeline* pipeline;
        std::string vertShaderFile;
        std::string fragShaderFile;
    public:
        RendererAbstract(Device* _device, RenderSystem* _renderSystem);
        void init(const unsigned int& renderPassIndex, const unsigned int& subPassIndex);
        virtual ~RendererAbstract();
        virtual void render(FrameInfo& frameInfo) = 0;
    };
    struct DefaultPushConstantData {
        glm::mat4 modelMat{1.0f};
        glm::mat4 normalMat{1.0f};
    };

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
        DiffuseBlinnPhongRenderer(Device* _device, RenderSystem* _renderSystem,
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
        PointLightRenderer(Device* _device, RenderSystem* _renderSystem, VkDescriptorSetLayout& globalDescriptorSetLayout);
        PointLightRenderer(const PointLightRenderer& copy) = delete;
        PointLightRenderer& operator=(const PointLightRenderer& copy) = delete;
        PointLightRenderer(PointLightRenderer&& move) = delete;
        PointLightRenderer& operator=(PointLightRenderer&& move) = delete;
        virtual ~PointLightRenderer() = default;
        
        void render(FrameInfo& frameInfo);
    };

    class PostProcessRenderer : public RendererAbstract {
    public:
        PostProcessRenderer(Device* _device, RenderSystem* _renderSystem);
        PostProcessRenderer(const PostProcessRenderer& copy) = delete;
        PostProcessRenderer& operator=(const PostProcessRenderer& copy) = delete;
        PostProcessRenderer(PostProcessRenderer&& move) = delete;
        PostProcessRenderer& operator=(PostProcessRenderer&& move) = delete;
        virtual ~PostProcessRenderer() = default;
        
        void render(FrameInfo& frameInfo);
    };
}

#endif// ENG_RENDERSYSTEMS