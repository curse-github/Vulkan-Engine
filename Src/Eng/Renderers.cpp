#include "Renderers.h"

namespace Eng {
    DiffuseBlinnPhongRenderer::DiffuseBlinnPhongRenderer(
        Device* _device, VkDescriptorSetLayout _globalDescriptorSetLayout, ResourceManager* _resourceManager
    ) : RendererAbstract(_device, _globalDescriptorSetLayout, _resourceManager)
    {
        // define uniforms
        materialIndexUniform = resourceManager->getMappedUniform(1, sizeof(unsigned int), 256, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_FRAGMENT_BIT, true);
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
        pipelineConfig.addFragSpecializationConstant(1, resourceManager->numTextures);
        pipelineConfig.addFragSpecializationConstant(2, resourceManager->numMaterials);
        // set shader file paths
        vertShaderFile = "shaders/Diffuse-Blinn-Phong.vert.spv";
        fragShaderFile = "shaders/Diffuse-Blinn-Phong.frag.spv";
        construct();
    }
    void DiffuseBlinnPhongRenderer::construct() {
        RendererAbstract::construct();
        // add uniforms
        descriptorSetLayouts.push_back(resourceManager->materialDescriptorSetLayout->descriptorSetLayout);
        descriptorSetLayouts.push_back(materialIndexUniform->setLayout[0].descriptorSetLayout);
    }
    void DiffuseBlinnPhongRenderer::render(FrameInfo& frameInfo) {
        pipeline->bind(frameInfo.commandBuffer);
        std::vector<VkDescriptorSet> descriptorSets{frameInfo.globalDescriptorSet, resourceManager->materialDescriptorSet};
        vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, static_cast<unsigned int>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);
        std::vector<ECS_id_t>& meshRendereredEntityIds = frameInfo.entitySystem->GetEntitiesWithComponent<MeshRendererComponent>();
        for (size_t i = 0; i < meshRendereredEntityIds.size(); i++) {
            Entity& entity = frameInfo.entitySystem->GetEntity(meshRendereredEntityIds[i]);
            TransformComponent& transform = entity.GetComponent<TransformComponent>();
            MeshRendererComponent& meshRenderer = entity.GetComponent<MeshRendererComponent>();
            unsigned int materialIdx = resourceManager->getMaterialIdx(meshRenderer.material);
            Mesh* mesh = resourceManager->getMesh(meshRenderer.mesh);

            materialIndexUniform->buffers[0]->writeAtIndex(&materialIdx, i);
            unsigned int dynamicOffset = materialIndexUniform->buffers[0]->getOffsetOfIndex(i);
            vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, static_cast<unsigned int>(descriptorSets.size()), 1, &materialIndexUniform->sets[0], 1, &dynamicOffset);
            DefaultPushConstantData pushVert{transform.getTransformMat(), transform.getNormalMat()};
            vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(DefaultPushConstantData), &pushVert);
            mesh->draw(frameInfo.commandBuffer);
        }
    }
    
    PointLightRenderer::PointLightRenderer(
        Device* _device, VkDescriptorSetLayout _globalDescriptorSetLayout, ResourceManager* _resourceManager
    ) : RendererAbstract(_device, _globalDescriptorSetLayout, _resourceManager) {
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
    
    OnTilePostProcessRenderer::OnTilePostProcessRenderer(
        Device* _device, VkDescriptorSetLayout _globalDescriptorSetLayout, ResourceManager* _resourceManager, const std::string& pixelShader
    ) : RendererAbstract(_device, _globalDescriptorSetLayout, _resourceManager) {
        // set shader file paths
        vertShaderFile = "shaders/FullScreen.vert.spv";
        fragShaderFile = pixelShader + ".spv";
        construct();
    }
    void OnTilePostProcessRenderer::render(FrameInfo& frameInfo) {
        pipeline->bind(frameInfo.commandBuffer);
        vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &frameInfo.globalDescriptorSet, 0, nullptr);
        vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
    }
    
    OffTilePostProcessRenderer::OffTilePostProcessRenderer(
        Device* _device, VkDescriptorSetLayout _globalDescriptorSetLayout, ResourceManager* _resourceManager, const std::string& pixelShader
    ) : RendererAbstract(_device, _globalDescriptorSetLayout, _resourceManager) {
        // set shader file paths
        vertShaderFile = "shaders/FullScreen.vert.spv";
        fragShaderFile = pixelShader + ".spv";
        construct();
    }
    void OffTilePostProcessRenderer::render(FrameInfo& frameInfo) {
        pipeline->bind(frameInfo.commandBuffer);
        vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &frameInfo.globalDescriptorSet, 0, nullptr);
        vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
    }
}