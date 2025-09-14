#include "RenderSystems.h"

namespace Eng {
    DiffuseBlinnPhongRenderSystem::DiffuseBlinnPhongRenderSystem(Device* _device, VkRenderPass renderPass,
        VkDescriptorSetLayout& globalDescriptorSetLayout, VkDescriptorSetLayout& materialDescriptorSetLayout, const unsigned int& numTextures, const unsigned int& numMaterials, DescriptorPool* globalDescriptorPool
    ) : device(_device), pipeline(nullptr)
    {
        // define push constants
        std::vector<VkPushConstantRange> pushConstantRanges(1);
        pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRanges[0].offset = 0;
        pushConstantRanges[0].size = sizeof(DefaultPushConstantData);
        // define uniforms
        materialIndexDescriptorSetLayout = DescriptorSetLayout::Builder(device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_FRAGMENT_BIT, 1).build();
        materialIndexUniformBuffer = new Buffer(device, sizeof(unsigned int), 256, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, device->properties.limits.minUniformBufferOffsetAlignment);
        materialIndexUniformBuffer->map();
        VkDescriptorBufferInfo materialUniformBufferDescriptor = materialIndexUniformBuffer->descriptorInfo(materialIndexUniformBuffer->paddedInstaceSize);
        DescriptorWriter(materialIndexDescriptorSetLayout, globalDescriptorPool)
            .writeBuffer(0, &materialUniformBufferDescriptor).build(materialIndexDescriptorSet);
        // create layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        // used for sending any data to GPU, besides vertex data.
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalDescriptorSetLayout, materialDescriptorSetLayout, materialIndexDescriptorSetLayout->descriptorSetLayout};
        pipelineLayoutInfo.setLayoutCount = static_cast<unsigned int>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        // used for sending small amounts of data.
        pipelineLayoutInfo.pushConstantRangeCount = static_cast<unsigned int>(pushConstantRanges.size());
        pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();
        if (vkCreatePipelineLayout(device->device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create pipeline layout!");
        // create pipeline
        assert((pipelineLayout != VK_NULL_HANDLE) && "cannot create pipeline before pipeline layout.");
        if (pipeline != nullptr) delete pipeline;
        PipelineConfigInfo pipelineConfig{};
        Pipeline::configSetDefaults(pipelineConfig);
        Pipeline::configEnableAlphaBlending(pipelineConfig);// temporary
        pipelineConfig.bindingDescriptions = Mesh::Vertex::getBindingDescriptions();
        pipelineConfig.attributeDescriptions = Mesh::Vertex::getAttributeDescriptions();
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;
        // specialization info
        // give MAX_LIGHTS to the vert and frag shader
        unsigned int temp1 = MAX_LIGHTS;
        pipelineConfig.vertSpecializationInfoEntries.push_back(VkSpecializationMapEntry{0, 0, sizeof(temp1)});
        pipelineConfig.vertSpecializationInfoData.insert(pipelineConfig.vertSpecializationInfoData.cend(), (char*)&temp1, ((char*)&temp1)+sizeof(temp1));
        pipelineConfig.fragSpecializationInfoEntries.push_back(VkSpecializationMapEntry{0, 0, sizeof(temp1)});
        pipelineConfig.fragSpecializationInfoData.insert(pipelineConfig.fragSpecializationInfoData.cend(), (char*)&numTextures, ((char*)&numTextures)+sizeof(numTextures));
        // give NUM_TEXTURES to the frag shader
        temp1 = numTextures;
        pipelineConfig.fragSpecializationInfoEntries.push_back(VkSpecializationMapEntry{1, sizeof(temp1), sizeof(numTextures)});
        pipelineConfig.fragSpecializationInfoData.insert(pipelineConfig.fragSpecializationInfoData.cend(), (char*)&temp1, ((char*)&temp1)+sizeof(temp1));
        // give NUM_MATERIALS to the frag shader
        temp1 = numMaterials;
        pipelineConfig.fragSpecializationInfoEntries.push_back(VkSpecializationMapEntry{2, sizeof(temp1)+sizeof(numTextures), sizeof(numMaterials)});
        pipelineConfig.fragSpecializationInfoData.insert(pipelineConfig.fragSpecializationInfoData.cend(), (char*)&temp1, ((char*)&temp1)+sizeof(temp1));
        // create actual pipeline
        pipeline = new Pipeline(device, "shaders/Diffuse-Blinn-Phong.vert.spv", "shaders/Diffuse-Blinn-Phong.frag.spv", pipelineConfig);
    }

    DiffuseBlinnPhongRenderSystem::~DiffuseBlinnPhongRenderSystem() {
        delete pipeline;
        vkDestroyPipelineLayout(device->device, pipelineLayout, nullptr);
    }
    void DiffuseBlinnPhongRenderSystem::recordObjects(FrameInfo& frameInfo) {
        pipeline->bind(frameInfo.commandBuffer);
        std::vector<VkDescriptorSet> sets{frameInfo.globalDescriptorSet, frameInfo.materialDescriptorSet};
        vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, static_cast<unsigned int>(sets.size()), sets.data(), 0, nullptr);
        std::vector<ECS_id_t>& meshRendereredEntityIds = frameInfo.entitySystem->GetEntitiesWithComponent<MeshRendererComponent>();
        for (size_t i = 0; i < meshRendereredEntityIds.size(); i++) {
            Entity& entity = frameInfo.entitySystem->GetEntity(meshRendereredEntityIds[i]);
            TransformComponent& transform = entity.GetComponent<TransformComponent>();
            MeshRendererComponent& meshRenderer = entity.GetComponent<MeshRendererComponent>();
            materialIndexUniformBuffer->writeAtIndex(&meshRenderer.materialIdx, i);
            materialIndexUniformBuffer->flushAtIndex(i);
            unsigned int dynamicOffset = i*materialIndexUniformBuffer->paddedInstaceSize;
            vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, static_cast<unsigned int>(sets.size()), 1, &materialIndexDescriptorSet, 1, &dynamicOffset);
            DefaultPushConstantData pushVert{transform.getTransformMat(), transform.getNormalMat()};
            vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(DefaultPushConstantData), &pushVert);
            meshRenderer.mesh->bind(frameInfo.commandBuffer);
            meshRenderer.mesh->draw(frameInfo.commandBuffer);
        }
    }

    
    PointLightRenderSystem::PointLightRenderSystem(Device* _device, VkRenderPass renderPass, VkDescriptorSetLayout& globalDescriptorSetLayout)
        : device(_device), pipeline(nullptr)
    {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(PointLightPushConstantData);
        // create layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        // used for sending any data to GPU, besides vertex data.
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &globalDescriptorSetLayout;
        // used for sending small amounts of data.
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        if (vkCreatePipelineLayout(device->device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create pipeline layout!");
        // create pipeline
        assert((pipelineLayout != VK_NULL_HANDLE) && "cannot create pipeline before pipeline layout.");
        if (pipeline != nullptr) delete pipeline;
        PipelineConfigInfo pipelineConfig{};
        Pipeline::configSetDefaults(pipelineConfig);
        Pipeline::configEnableAlphaBlending(pipelineConfig);
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;
        // specialization info
        unsigned int temp = MAX_LIGHTS;
        pipelineConfig.vertSpecializationInfoEntries.push_back(VkSpecializationMapEntry{0, 0, sizeof(temp)});
        pipelineConfig.vertSpecializationInfoData.insert(pipelineConfig.vertSpecializationInfoData.cend(), (char*)&temp, ((char*)&temp)+sizeof(temp));
        pipelineConfig.fragSpecializationInfoEntries.push_back(VkSpecializationMapEntry{0, 0, sizeof(temp)});
        pipelineConfig.fragSpecializationInfoData.insert(pipelineConfig.fragSpecializationInfoData.cend(), (char*)&temp, ((char*)&temp)+sizeof(temp));
        // create actual pipeline
        pipeline = new Pipeline(device, "shaders/PointLight.vert.spv", "shaders/PointLight.frag.spv", pipelineConfig);
    }

    PointLightRenderSystem::~PointLightRenderSystem() {
        delete pipeline;
        vkDestroyPipelineLayout(device->device, pipelineLayout, nullptr);
    }
#define sortedGameObjectIdsT std::map<float, GameObject::id_t>
    void PointLightRenderSystem::recordObjects(FrameInfo& frameInfo) {
        pipeline->bind(frameInfo.commandBuffer);
        vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &frameInfo.globalDescriptorSet, 0, nullptr);
        std::vector<ECS_id_t>& lightsEntityIds = frameInfo.entitySystem->GetEntitiesWithComponent<PointLightComponent>();
        for (size_t i = 0; i < lightsEntityIds.size(); i++) {
            Entity& entity = frameInfo.entitySystem->GetEntity(lightsEntityIds[i]);
            TransformComponent& transform = entity.GetComponent<TransformComponent>();
            PointLightComponent& pointLight = entity.GetComponent<PointLightComponent>();
            PointLightPushConstantData push{vec4(transform.position, transform.scale.x), pointLight.colorIntensity};
            vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PointLightPushConstantData), &push);
            vkCmdDraw(frameInfo.commandBuffer, 3, 1, 0, 0);
        }
    }
}