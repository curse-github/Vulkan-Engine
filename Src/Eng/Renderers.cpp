#include "Renderers.h"

namespace Eng {
    DiffuseBlinnPhongRenderer::DiffuseBlinnPhongRenderer(Device* _device, const unsigned int& numTextures, const unsigned int& numMaterials,
            VkDescriptorSetLayout& _globalDescriptorSetLayout, VkDescriptorSetLayout& _materialDescriptorSetLayout, DescriptorPool* _globalDescriptorPool
    ) : RendererAbstract(_device), globalDescriptorSetLayout(_globalDescriptorSetLayout), materialDescriptorSetLayout(_materialDescriptorSetLayout), globalDescriptorPool(_globalDescriptorPool)
    {
        // add push constants
        pushConstantRanges.push_back(VkPushConstantRange{
            VK_SHADER_STAGE_VERTEX_BIT,
            0, sizeof(DefaultPushConstantData)
        });
        // setup pipeline config
        pipelineConfig.enableAlphaBlending();
        pipelineConfig.bindingDescriptions = Mesh::Vertex::getBindingDescriptions();
        pipelineConfig.attributeDescriptions = Mesh::Vertex::getAttributeDescriptions();
        pipelineConfig.addVertSpecializationConstant(0, MAX_LIGHTS);
        pipelineConfig.addFragSpecializationConstant(0, MAX_LIGHTS);
        pipelineConfig.addFragSpecializationConstant(1, numTextures);
        pipelineConfig.addFragSpecializationConstant(2, numMaterials);
        // set shader file paths
        vertShaderFile = "shaders/Diffuse-Blinn-Phong.vert.spv";
        fragShaderFile = "shaders/Diffuse-Blinn-Phong.frag.spv";
        construct();
    }
    void DiffuseBlinnPhongRenderer::construct() {
        RendererAbstract::construct();
        // add uniforms
        materialIndexDescriptorSetLayout = DescriptorSetLayout::Builder(device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_FRAGMENT_BIT, 1).build();
        materialIndexUniformBuffer = new Buffer(device, sizeof(unsigned int), 256, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, device->properties.limits.minUniformBufferOffsetAlignment);
        materialIndexUniformBuffer->map();
        VkDescriptorBufferInfo materialUniformBufferDescriptor = materialIndexUniformBuffer->descriptorInfo(materialIndexUniformBuffer->paddedInstaceSize);
        DescriptorWriter(materialIndexDescriptorSetLayout, globalDescriptorPool)
            .writeBuffer(0, &materialUniformBufferDescriptor).build(materialIndexDescriptorSet);
        descriptorSetLayouts.push_back(globalDescriptorSetLayout);
        descriptorSetLayouts.push_back(materialDescriptorSetLayout);
        descriptorSetLayouts.push_back(materialIndexDescriptorSetLayout->descriptorSetLayout);
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
    
    PointLightRenderer::PointLightRenderer(Device* _device, VkDescriptorSetLayout& _globalDescriptorSetLayout)
        : RendererAbstract(_device), globalDescriptorSetLayout(_globalDescriptorSetLayout)
    {
        // add push constants
        pushConstantRanges.push_back(VkPushConstantRange{
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0, sizeof(PointLightPushConstantData)
        });
        pipelineConfig.enableAlphaBlending();
        pipelineConfig.addVertSpecializationConstant(0, MAX_LIGHTS);
        pipelineConfig.addFragSpecializationConstant(0, MAX_LIGHTS);
        // set shader file paths
        vertShaderFile = "shaders/PointLight.vert.spv";
        fragShaderFile = "shaders/PointLight.frag.spv";
        construct();
    }
    void PointLightRenderer::construct() {
        RendererAbstract::construct();
        // add uniforms
        descriptorSetLayouts.push_back(globalDescriptorSetLayout);
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
    
    OnTilePostProcessRenderer::OnTilePostProcessRenderer(Device* _device, const std::string& pixelShader, VkDescriptorSetLayout& _globalDescriptorSetLayout)
        : RendererAbstract(_device), globalDescriptorSetLayout(_globalDescriptorSetLayout)
    {
        // set shader file paths
        vertShaderFile = "shaders/FullScreen.vert.spv";
        fragShaderFile = pixelShader + ".spv";
        construct();
    }
    void OnTilePostProcessRenderer::construct() {
        RendererAbstract::construct();
        // add uniforms
        descriptorSetLayouts.push_back(globalDescriptorSetLayout);
    }
    void OnTilePostProcessRenderer::render(FrameInfo& frameInfo) {
        pipeline->bind(frameInfo.commandBuffer);
        vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &frameInfo.globalDescriptorSet, 0, nullptr);
        vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
    }
    
    OffTilePostProcessRenderer::OffTilePostProcessRenderer(Device* _device, const std::string& pixelShader, VkDescriptorSetLayout& _globalDescriptorSetLayout)
        : RendererAbstract(_device), globalDescriptorSetLayout(_globalDescriptorSetLayout)
    {
        // set shader file paths
        vertShaderFile = "shaders/FullScreen.vert.spv";
        fragShaderFile = pixelShader + ".spv";
        construct();
    }
    void OffTilePostProcessRenderer::construct() {
        RendererAbstract::construct();
        // add uniforms
        descriptorSetLayouts.push_back(globalDescriptorSetLayout);
    }
    void OffTilePostProcessRenderer::render(FrameInfo& frameInfo) {
        pipeline->bind(frameInfo.commandBuffer);
        vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &frameInfo.globalDescriptorSet, 0, nullptr);
        vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
    }
}