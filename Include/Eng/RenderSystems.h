#ifndef ENG_RENDERSYSTEMS
#define ENG_RENDERSYSTEMS

#include "Helpers.h"
#include "Device.h"
#include "Pipeline.h"
#include "Mesh.h"
#include "FrameInfo.h"
#include "UboStructs.h"
#include "Descriptors.h"
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
    struct DefaultPushConstantData {
        glm::mat4 modelMat{1.0f};
        glm::mat4 normalMat{1.0f};
    };
    class DiffuseBlinnPhongRenderSystem {
        Device* device;
        Pipeline* pipeline;
        VkPipelineLayout pipelineLayout;
        OwnedPointer<DescriptorSetLayout> materialIndexDescriptorSetLayout;
        VkDescriptorSet materialIndexDescriptorSet;
        OwnedPointer<Buffer> materialIndexUniformBuffer;

        void recordCommandBuffer(const int& imageIndex);
    public:
        DiffuseBlinnPhongRenderSystem(Device* _device, VkRenderPass renderPass,
            VkDescriptorSetLayout& globalDescriptorSetLayout, VkDescriptorSetLayout& materialDescriptorSetLayout, const unsigned int& numTextures, const unsigned int& numMaterials, DescriptorPool* globalDescriptorPool);
        DiffuseBlinnPhongRenderSystem(const DiffuseBlinnPhongRenderSystem& copy) = delete;
        DiffuseBlinnPhongRenderSystem& operator=(const DiffuseBlinnPhongRenderSystem& copy) = delete;
        DiffuseBlinnPhongRenderSystem(DiffuseBlinnPhongRenderSystem&& move) = delete;
        DiffuseBlinnPhongRenderSystem& operator=(DiffuseBlinnPhongRenderSystem&& move) = delete;
        ~DiffuseBlinnPhongRenderSystem();
        
        void recordObjects(FrameInfo& frameInfo);
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
    class PointLightRenderSystem {
        Device* device;
        Pipeline* pipeline;
        VkPipelineLayout pipelineLayout;

        void recordCommandBuffer(const int& imageIndex);
    public:
        PointLightRenderSystem(Device* _device, VkRenderPass renderPass, VkDescriptorSetLayout& globalDescriptorSetLayout);
        PointLightRenderSystem(const PointLightRenderSystem& copy) = delete;
        PointLightRenderSystem& operator=(const PointLightRenderSystem& copy) = delete;
        PointLightRenderSystem(PointLightRenderSystem&& move) = delete;
        PointLightRenderSystem& operator=(PointLightRenderSystem&& move) = delete;
        ~PointLightRenderSystem();
        
        void recordObjects(FrameInfo& frameInfo);
    };
}

#endif// ENG_RENDERSYSTEMS