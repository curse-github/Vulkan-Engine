#include "Renderers.h"

namespace Eng {
    RendererAbstract::RendererAbstract(Device* _device, RenderSystem* _renderSystem) : device(_device), renderSystem(_renderSystem), pipeline(nullptr) {
        pipelineConfig.setDefaults();
    };
    void RendererAbstract::init(const unsigned int& renderPassIndex, const unsigned int& subPassIndex) {
        // create actual pipeline
        if (vkCreatePipelineLayout(device->device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create pipeline layout!");
        pipelineConfig.renderPass = renderSystem->getRenderPass(renderPassIndex);
        pipelineConfig.subpass = subPassIndex;
        pipelineConfig.pipelineLayout = pipelineLayout;
        pipeline = new Pipeline(device, vertShaderFile, fragShaderFile, pipelineConfig);
    }
    RendererAbstract::~RendererAbstract() {
        delete pipeline;
        vkDestroyPipelineLayout(device->device, pipelineLayout, nullptr);
    };

    DiffuseBlinnPhongRenderer::DiffuseBlinnPhongRenderer(Device* _device, RenderSystem* _renderSystem,
            VkDescriptorSetLayout& globalDescriptorSetLayout, VkDescriptorSetLayout& materialDescriptorSetLayout, const unsigned int& numTextures, const unsigned int& numMaterials, DescriptorPool* globalDescriptorPool
    ) : RendererAbstract(_device, _renderSystem)
    {
        // define uniforms
        materialIndexDescriptorSetLayout = DescriptorSetLayout::Builder(device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_FRAGMENT_BIT, 1).build();
        materialIndexUniformBuffer = new Buffer(device, sizeof(unsigned int), 256, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, device->properties.limits.minUniformBufferOffsetAlignment);
        materialIndexUniformBuffer->map();
        VkDescriptorBufferInfo materialUniformBufferDescriptor = materialIndexUniformBuffer->descriptorInfo(materialIndexUniformBuffer->paddedInstaceSize);
        DescriptorWriter(materialIndexDescriptorSetLayout, globalDescriptorPool)
            .writeBuffer(0, &materialUniformBufferDescriptor).build(materialIndexDescriptorSet);
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalDescriptorSetLayout, materialDescriptorSetLayout, materialIndexDescriptorSetLayout->descriptorSetLayout};
        // define push constants
        std::vector<VkPushConstantRange> pushConstantRanges(1);
        pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRanges[0].offset = 0;
        pushConstantRanges[0].size = sizeof(DefaultPushConstantData);
        // setup pipeline layout
        pipelineLayoutCreateInfo.setLayoutCount = static_cast<unsigned int>(descriptorSetLayouts.size());
        pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<unsigned int>(pushConstantRanges.size());
        pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();
        // setup pipeline config
        pipelineConfig.enableAlphaBlending();
        pipelineConfig.bindingDescriptions = Mesh::Vertex::getBindingDescriptions();
        pipelineConfig.attributeDescriptions = Mesh::Vertex::getAttributeDescriptions();
        pipelineConfig.addVertSpecializationConstant(0, MAX_LIGHTS);
        pipelineConfig.addFragSpecializationConstant(0, MAX_LIGHTS);
        pipelineConfig.addFragSpecializationConstant(1, numTextures);
        pipelineConfig.addFragSpecializationConstant(2, numMaterials);
        // create pipeline
        vertShaderFile = "shaders/Diffuse-Blinn-Phong.vert.spv";
        fragShaderFile = "shaders/Diffuse-Blinn-Phong.frag.spv";
        init(0, 0);
    }
    void DiffuseBlinnPhongRenderer::render(FrameInfo& frameInfo) {
        pipeline->bind(frameInfo.commandBuffer);
        std::vector<VkDescriptorSet> descriptorSets{frameInfo.globalDescriptorSet, frameInfo.materialDescriptorSet};
        vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, static_cast<unsigned int>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);
        std::vector<ECS_id_t>& meshRendereredEntityIds = frameInfo.entitySystem->GetEntitiesWithComponent<MeshRendererComponent>();
        for (size_t i = 0; i < meshRendereredEntityIds.size(); i++) {
            Entity& entity = frameInfo.entitySystem->GetEntity(meshRendereredEntityIds[i]);
            TransformComponent& transform = entity.GetComponent<TransformComponent>();
            MeshRendererComponent& meshRenderer = entity.GetComponent<MeshRendererComponent>();
            materialIndexUniformBuffer->writeAtIndex(&meshRenderer.materialIdx, i);
            materialIndexUniformBuffer->flushAtIndex(i);
            unsigned int dynamicOffset = i*materialIndexUniformBuffer->paddedInstaceSize;
            vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, static_cast<unsigned int>(descriptorSets.size()), 1, &materialIndexDescriptorSet, 1, &dynamicOffset);
            DefaultPushConstantData pushVert{transform.getTransformMat(), transform.getNormalMat()};
            vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(DefaultPushConstantData), &pushVert);
            meshRenderer.mesh->bind(frameInfo.commandBuffer);
            meshRenderer.mesh->draw(frameInfo.commandBuffer);
        }
    }
    
    PointLightRenderer::PointLightRenderer(Device* _device, RenderSystem* _renderSystem, VkDescriptorSetLayout& globalDescriptorSetLayout)
        : RendererAbstract(_device, _renderSystem)
    {
        // create uniforms
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalDescriptorSetLayout};
        // create push constants
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(PointLightPushConstantData);
        // setup pipeline layout
        pipelineLayoutCreateInfo.setLayoutCount = static_cast<unsigned int>(descriptorSetLayouts.size());
        pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
        pipelineConfig.enableAlphaBlending();
        pipelineConfig.addVertSpecializationConstant(0, MAX_LIGHTS);
        pipelineConfig.addFragSpecializationConstant(0, MAX_LIGHTS);
        // create pipeline
        vertShaderFile = "shaders/PointLight.vert.spv";
        fragShaderFile = "shaders/PointLight.frag.spv";
        init(0,0);
    }
    void PointLightRenderer::render(FrameInfo& frameInfo) {
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
    
    PostProcessRenderer::PostProcessRenderer(Device* _device, RenderSystem* _renderSystem) : RendererAbstract(_device, _renderSystem) {
        // create uniforms
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{renderSystem->inputAttachmentDescriptorSetLayout->descriptorSetLayout};
        // create push constants
        std::vector<VkPushConstantRange> pushConstantRanges{};
        // setup pipeline layout
        pipelineLayoutCreateInfo.setLayoutCount = static_cast<unsigned int>(descriptorSetLayouts.size());
        pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<unsigned int>(pushConstantRanges.size());
        pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();
        // create pipeline
        vertShaderFile = "shaders/FullScreen.vert.spv";
        fragShaderFile = "shaders/PostProcess.frag.spv";
        init(0, 1);
    }
    void PostProcessRenderer::render(FrameInfo& frameInfo) {
        std::vector<VkDescriptorSet> descriptorSets{renderSystem->inputAttachmentDescriptorSets[frameInfo.imageIndex]};
        vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, static_cast<unsigned int>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);
        pipeline->bind(frameInfo.commandBuffer);
        vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
    }
}