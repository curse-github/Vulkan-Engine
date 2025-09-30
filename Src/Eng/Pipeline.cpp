#include "Pipeline.h"

namespace Eng {
    void PipelineConfig::setDefaults() {
        // viewportInfo
        viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportInfo.viewportCount = 1;
        viewportInfo.pViewports = nullptr;
        viewportInfo.scissorCount = 1;
        viewportInfo.pScissors = nullptr;
        // inputAssemblyInfo
        inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;// IMPORTANT, might be changed soon
        inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
        // rasterizationInfo
        rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationInfo.depthBiasClamp = VK_FALSE;
        rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationInfo.lineWidth = 1.0f;
        rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizationInfo.depthBiasEnable = VK_FALSE;
        rasterizationInfo.depthBiasConstantFactor = 0.0f; // Optional, since it is just set to 0;
        rasterizationInfo.depthBiasClamp = 0.0f;          // Optional, since it is just set to 0;
        rasterizationInfo.depthBiasSlopeFactor = 0.0f;    // Optional, since it is just set to 0;
        // multisampleInfo
        multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleInfo.sampleShadingEnable = VK_FALSE;
        multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampleInfo.minSampleShading = 1.0f;          // Optional, since multisampling is off
        multisampleInfo.pSampleMask = nullptr;            // Optional, since multisampling is off
        multisampleInfo.alphaToCoverageEnable = VK_FALSE; // Optional, since multisampling is off
        multisampleInfo.alphaToOneEnable = VK_FALSE;      // Optional, since multisampling is off
        // colorBlend attachment and info
        colorBlendAttachments.push_back(defaultColorBlendState());
        // depthStencilInfo
        depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilInfo.depthTestEnable = VK_TRUE;
        depthStencilInfo.depthWriteEnable = VK_TRUE;
        depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
        depthStencilInfo.minDepthBounds = 0.0f;            // Optional, since depth bounds test is disabled
        depthStencilInfo.maxDepthBounds = 1.0f;            // Optional, since depth bounds test is disabled
        depthStencilInfo.stencilTestEnable = VK_FALSE;
        depthStencilInfo.front = {};
        depthStencilInfo.back = {};
        // dynamicStateEnables
        dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateInfo.pDynamicStates = dynamicStateEnables.data();
        dynamicStateInfo.dynamicStateCount = static_cast<unsigned int>(dynamicStateEnables.size());
    }
    void PipelineConfig::enableAlphaBlending() {
        colorBlendAttachments[0] = alphaBlendingColorBlendState();
        depthStencilInfo.depthWriteEnable = VK_FALSE;
    }




    void PipelineConfig::addVertSpecializationConstant(const unsigned int& constant_id, const unsigned int& value) {
        vertSpecializationInfoEntries.push_back(VkSpecializationMapEntry{constant_id, static_cast<unsigned int>(sizeof(unsigned char)*vertSpecializationInfoData.size()), sizeof(unsigned int)});
        vertSpecializationInfoData.insert(vertSpecializationInfoData.cend(), (char*)&value, ((char*)&value)+sizeof(unsigned int));
    };
    void PipelineConfig::addFragSpecializationConstant(const unsigned int& constant_id, const unsigned int& value) {
        fragSpecializationInfoEntries.push_back(VkSpecializationMapEntry{constant_id, static_cast<unsigned int>(sizeof(unsigned char)*fragSpecializationInfoData.size()), sizeof(unsigned int)});
        fragSpecializationInfoData.insert(fragSpecializationInfoData.cend(), (char*)&value, ((char*)&value)+sizeof(unsigned int));
    };
    Pipeline::Pipeline(Device* _device, const std::string& vert, const std::string& frag, const PipelineConfig& config) : device(_device) {
        assert((config.pipelineLayout != VK_NULL_HANDLE) && "Cannot create graphics pipeline:: no pipelineLayout provided in config");
        assert((config.renderPass != VK_NULL_HANDLE) && "Cannot create graphics pipeline:: no renderPass provided in config");
        std::vector<char> vertCode = readFile(vert);
        std::vector<char> fragCode = readFile(frag);
/*#if defined(_DEBUG) && (_DEBUG==1)
        std::cout << "vertCode length = " << vertCode.size() << '\n';
        std::cout << "fragCode length = " << fragCode.size() << '\n';
#endif*/

        createShaderModule(vertCode, &vertShaderModule);
        createShaderModule(fragCode, &fragShaderModule);
        VkPipelineShaderStageCreateInfo shaderStages[2];
        shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[0].pNext = nullptr;
        shaderStages[0].flags = 0;
        shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[0].module = vertShaderModule;
        shaderStages[0].pName = "main";
        VkSpecializationInfo vertSpecializationInfo{};
        vertSpecializationInfo.mapEntryCount = static_cast<unsigned int>(config.vertSpecializationInfoEntries.size());
        if (vertSpecializationInfo.mapEntryCount > 0) {
            vertSpecializationInfo.pMapEntries = config.vertSpecializationInfoEntries.data();
            vertSpecializationInfo.dataSize = config.vertSpecializationInfoData.size();
            vertSpecializationInfo.pData = config.vertSpecializationInfoData.data();
            shaderStages[0].pSpecializationInfo = &vertSpecializationInfo;
        } else
            shaderStages[0].pSpecializationInfo = nullptr;
        shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].pNext = nullptr;
        shaderStages[1].flags = 0;
        shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].module = fragShaderModule;
        shaderStages[1].pName = "main";
        VkSpecializationInfo fragSpecializationInfo{};
        fragSpecializationInfo.mapEntryCount = static_cast<unsigned int>(config.fragSpecializationInfoEntries.size());
        if (fragSpecializationInfo.mapEntryCount > 0) {
            fragSpecializationInfo.pMapEntries = config.fragSpecializationInfoEntries.data();
            fragSpecializationInfo.dataSize = config.fragSpecializationInfoData.size();
            fragSpecializationInfo.pData = config.fragSpecializationInfoData.data();
            shaderStages[1].pSpecializationInfo = &fragSpecializationInfo;
        } else
            shaderStages[1].pSpecializationInfo = nullptr;

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<unsigned int>(config.attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = config.attributeDescriptions.data();
        vertexInputInfo.vertexBindingDescriptionCount = static_cast<unsigned int>(config.bindingDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = config.bindingDescriptions.data();
        
        VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
        colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendInfo.logicOpEnable = VK_FALSE;
        colorBlendInfo.logicOp = VK_LOGIC_OP_COPY; // Optional, since logic operation is disabled
        colorBlendInfo.attachmentCount = static_cast<unsigned int>(config.colorBlendAttachments.size());
        colorBlendInfo.pAttachments = config.colorBlendAttachments.data();
        colorBlendInfo.blendConstants[0] = 0.0f; // Optional, since it is just set to 0;
        colorBlendInfo.blendConstants[1] = 0.0f; // Optional, since it is just set to 0;
        colorBlendInfo.blendConstants[2] = 0.0f; // Optional, since it is just set to 0;
        colorBlendInfo.blendConstants[3] = 0.0f; // Optional, since it is just set to 0;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pViewportState = &config.viewportInfo;
        pipelineInfo.pInputAssemblyState = &config.inputAssemblyInfo;
        pipelineInfo.pRasterizationState = &config.rasterizationInfo;
        pipelineInfo.pMultisampleState = &config.multisampleInfo;
        pipelineInfo.pColorBlendState = &colorBlendInfo;
        pipelineInfo.pDepthStencilState = &config.depthStencilInfo;
        pipelineInfo.pDynamicState = &config.dynamicStateInfo;
        pipelineInfo.layout = config.pipelineLayout;
        pipelineInfo.renderPass = config.renderPass;
        pipelineInfo.subpass = config.subpass;
        pipelineInfo.basePipelineIndex = -1;
        // VK_NULL_HANDLE is used for one pipeline to derive from another, which can be more efficient for the GPU
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        
        // VK_NULL_HANDLE is for a cache, used for optimization
        if (vkCreateGraphicsPipelines(device->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
            throw std::runtime_error("Failed to create graphics pipeline");
    }
    Pipeline::~Pipeline() {
        vkDestroyShaderModule(device->device, vertShaderModule, nullptr);
        vkDestroyShaderModule(device->device, fragShaderModule, nullptr);
        vkDestroyPipeline(device->device, graphicsPipeline, nullptr);
    }



    
    void Pipeline::createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const unsigned int*>(code.data());
        if (vkCreateShaderModule(device->device, &createInfo, nullptr, shaderModule) != VK_SUCCESS)
            throw std::runtime_error("Failed to create shader module");
    }
    void Pipeline::bind(VkCommandBuffer commandBuffer) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    }
}