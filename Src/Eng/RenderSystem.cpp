#include "RenderSystem.h"

namespace Eng {
    RendererAbstract::RendererAbstract(Device* _device) : device(_device), pipeline(nullptr) {
        pipelineConfig.setDefaults();
    };
    void RendererAbstract::init(VkRenderPass& renderPass, const unsigned int& subPassIndex) {
        // put push constants and uniform descriptor layouts in pipline layout
        pipelineLayoutCreateInfo.setLayoutCount = static_cast<unsigned int>(descriptorSetLayouts.size());
        pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<unsigned int>(pushConstantRanges.size());
        pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();
        // create actual pipeline
        if (vkCreatePipelineLayout(device->device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create pipeline layout!");
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.subpass = subPassIndex;
        pipelineConfig.pipelineLayout = pipelineLayout;
        pipeline = new Pipeline(device, vertShaderFile, fragShaderFile, pipelineConfig);
    }
    RendererAbstract::~RendererAbstract() {
        delete pipeline;
        vkDestroyPipelineLayout(device->device, pipelineLayout, nullptr);
    };




    RenderSystem::RenderSystem(Window* _window, Device* _device, std::vector<std::vector<SubPass>>&& _passes, DescriptorPool* _globalDescriptorPool)
        : window(_window), device(_device), swapchain(nullptr), config((std::vector<std::vector<SubPass>>&&)_passes), globalDescriptorPool(_globalDescriptorPool)
    {
        recreateSwapchain();
        createCommandBuffers();
        allocateInputAttachments();
    }

    RenderSystem::~RenderSystem() {
        delete swapchain;
    }
    void RenderSystem::recreateSwapchain() {
        // IMPORTANT: this halts the program while minimized.
        while ((window->size.x == 0) || (window->size.y == 0))
            glfwWaitEvents();
        vkDeviceWaitIdle(device->device);
        // recreate the swapchain
        std::vector<std::vector<Swapchain::SubPassConfig>> subPassConfigs{};
        subPassConfigs.resize(config.passes.size());
        for (size_t i = 0; i < subPassConfigs.size(); i++) {
            subPassConfigs[i].reserve(config.passes[i].size());
            for (size_t j = 0; j < config.passes[i].size(); j++)
                subPassConfigs[i].push_back(config.passes[i][j].config);
        }
        if (swapchain == nullptr) swapchain = new Swapchain(device, VkExtent2D{static_cast<unsigned int>(window->size.x), static_cast<unsigned int>(window->size.y)}, subPassConfigs);
        else {
            std::cout << "Recreating swapchain\n";
            Swapchain* oldSwapchain = swapchain;
            swapchain = new Swapchain(device, VkExtent2D{static_cast<unsigned int>(window->size.x), static_cast<unsigned int>(window->size.y)}, subPassConfigs, oldSwapchain);
            delete oldSwapchain;
            if (!oldSwapchain->swapchainsCompatible(*swapchain))
                // should at some point just recreate the pipeline/rendersystems
                throw std::runtime_error("Swapchain image format has changed!");
            if (globalDescriptorPool == nullptr) throw std::runtime_error("rendersystem allocateInputAttachments was never called");
            overwriteInputAttachments();
        }
        scissor.extent = swapchain->swapChainExtent;
        viewport.width = static_cast<float>(scissor.extent.width);
        viewport.height = static_cast<float>(scissor.extent.height);
    }
    void RenderSystem::createCommandBuffers() {
        commandBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        // primary buffers can be submitted for execution but cannot be called by other buffers
        // secondary buffers cannot be submitted for execution directly but can be called by other buffers
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = device->commandPool;
        allocInfo.commandBufferCount = static_cast<unsigned int>(commandBuffers.size());
        if (vkAllocateCommandBuffers(device->device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate command buffers!");
    }
    void RenderSystem::freeCommandBuffers() {
        vkFreeCommandBuffers(device->device, device->commandPool, static_cast<unsigned int>(commandBuffers.size()), commandBuffers.data());
        commandBuffers.clear();
    }


    void RenderSystem::beginRenderPass(VkCommandBuffer commandBuffer, const unsigned int& _renderPassIndex) {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = swapchain->renderPasses[_renderPassIndex];
        renderPassInfo.framebuffer = swapchain->swapChainFramebuffers[_renderPassIndex][imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = {swapchain->swapChainExtent.width, swapchain->swapChainExtent.height};
        renderPassInfo.clearValueCount = static_cast<unsigned int>(swapchain->clearValues[_renderPassIndex].size());
        renderPassInfo.pClearValues = swapchain->clearValues[_renderPassIndex].data();
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        renderPassIndex = _renderPassIndex;
    }
    void RenderSystem::nextSubPass(VkCommandBuffer commandBuffer) {
        vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
    }
    void RenderSystem::endRenderPass(VkCommandBuffer commandBuffer) {
        vkCmdEndRenderPass(commandBuffer);
        renderPassIndex = ~0u;
    }

    void RenderSystem::allocateInputAttachments() {
        inputAttachmentDescriptorSetLayouts.resize(config.passes.size());
        sampledInputDescriptorSetLayouts.resize(config.passes.size());
        for (size_t i = 0; i < config.passes.size(); i++) {
            VkRenderPass renderPass = swapchain->renderPasses[i];
            size_t numSubPasses = config.passes[i].size();
            inputAttachmentDescriptorSetLayouts[i].resize(numSubPasses);
            sampledInputDescriptorSetLayouts[i].resize(numSubPasses);
            for (size_t j = 0; j < numSubPasses; j++) {
                size_t numInputsAttachments = config.passes[i][j].config.inputAttachmentIndices.size();
                if (numInputsAttachments > 0) {
                    DescriptorSetLayout::Builder layoutBuilder(device);
                    for (size_t k = 0; k < numInputsAttachments; k++)
                        layoutBuilder.addBinding(k, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
                    inputAttachmentDescriptorSetLayouts[i][j] = layoutBuilder.build();
                }
                size_t numSampledInputs = config.passes[i][j].config.sampledInputIndices.size();
                if (numSampledInputs > 0) {
                    DescriptorSetLayout::Builder layoutBuilder(device);
                    for (size_t k = 0; k < numSampledInputs; k++)
                        layoutBuilder.addBinding(k, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
                    sampledInputDescriptorSetLayouts[i][j] = layoutBuilder.build();
                }
                size_t numRenderers = config.passes[i][j].renderers.size();
                for (size_t k = 0; k < numRenderers; k++) {
                    if (numInputsAttachments > 0) config.passes[i][j].renderers[k]->descriptorSetLayouts.push_back(inputAttachmentDescriptorSetLayouts[i][j]->descriptorSetLayout);
                    if (numSampledInputs > 0) config.passes[i][j].renderers[k]->descriptorSetLayouts.push_back(sampledInputDescriptorSetLayouts[i][j]->descriptorSetLayout);
                    config.passes[i][j].renderers[k]->init(renderPass, j);
                }
            }
        }
        size_t numRenderPasses = config.passes.size();
        inputAttachmentDescriptorSets.resize(numRenderPasses);
        sampledInputDescriptorSets.resize(numRenderPasses);
        for (size_t i = 0; i < config.passes.size(); i++) {
            size_t numSubPasses = config.passes[i].size();
            inputAttachmentDescriptorSets[i].resize(numSubPasses);
            sampledInputDescriptorSets[i].resize(numSubPasses);
            for (size_t j = 0; j < numSubPasses; j++) {
                size_t numInputsAttachments = config.passes[i][j].config.inputAttachmentIndices.size();
                if (numInputsAttachments > 0) {
                    size_t imageCount = swapchain->getImageCount();
                    inputAttachmentDescriptorSets[i][j].resize(imageCount, VK_NULL_HANDLE);
                    for (size_t l = 0; l < swapchain->getImageCount(); l++) {
                        DescriptorWriter writer(inputAttachmentDescriptorSetLayouts[i][j], globalDescriptorPool);
                        for (size_t k = 0; k < numInputsAttachments; k++)
                            writer.writeImage(k, &swapchain->textureDescriptors[config.passes[i][j].config.inputAttachmentIndices[k]-1u][i]);
                        writer.build(inputAttachmentDescriptorSets[i][j][l]);
                    }
                }
                size_t numSampledInputs = config.passes[i][j].config.sampledInputIndices.size();
                if (numSampledInputs > 0) {
                    size_t imageCount = swapchain->getImageCount();
                    sampledInputDescriptorSets[i][j].resize(imageCount, VK_NULL_HANDLE);
                    for (size_t l = 0; l < swapchain->getImageCount(); l++) {
                        DescriptorWriter writer(sampledInputDescriptorSetLayouts[i][j], globalDescriptorPool);
                        for (size_t k = 0; k < numSampledInputs; k++)
                            writer.writeImage(k, &swapchain->textureDescriptors[config.passes[i][j].config.sampledInputIndices[k]-1u][i]);
                        writer.build(sampledInputDescriptorSets[i][j][l]);
                    }
                }
            }
        }
    }
    void RenderSystem::overwriteInputAttachments() {
        size_t numRenderPasses = config.passes.size();
        inputAttachmentDescriptorSets.resize(numRenderPasses);
        sampledInputDescriptorSets.resize(numRenderPasses);
        for (size_t i = 0; i < config.passes.size(); i++) {
            size_t numSubPasses = config.passes[i].size();
            inputAttachmentDescriptorSets[i].resize(numSubPasses);
            sampledInputDescriptorSets[i].resize(numSubPasses);
            for (size_t j = 0; j < numSubPasses; j++) {
                size_t numInputsAttachments = config.passes[i][j].config.inputAttachmentIndices.size();
                if (numInputsAttachments > 0) {
                    size_t imageCount = swapchain->getImageCount();
                    inputAttachmentDescriptorSets[i][j].resize(imageCount, VK_NULL_HANDLE);
                    for (size_t l = 0; l < swapchain->getImageCount(); l++) {
                        DescriptorWriter writer(inputAttachmentDescriptorSetLayouts[i][j], globalDescriptorPool);
                        for (size_t k = 0; k < numInputsAttachments; k++)
                            writer.writeImage(k, &swapchain->textureDescriptors[config.passes[i][j].config.inputAttachmentIndices[k]-1u][i]);
                        writer.overwrite(inputAttachmentDescriptorSets[i][j][l]);
                    }
                }
                size_t numSampledInputs = config.passes[i][j].config.sampledInputIndices.size();
                if (numSampledInputs > 0) {
                    size_t imageCount = swapchain->getImageCount();
                    sampledInputDescriptorSets[i][j].resize(imageCount, VK_NULL_HANDLE);
                    for (size_t l = 0; l < swapchain->getImageCount(); l++) {
                        DescriptorWriter writer(sampledInputDescriptorSetLayouts[i][j], globalDescriptorPool);
                        for (size_t k = 0; k < numSampledInputs; k++)
                            writer.writeImage(k, &swapchain->textureDescriptors[config.passes[i][j].config.sampledInputIndices[k]-1u][i]);
                        writer.overwrite(sampledInputDescriptorSets[i][j][l]);
                    }
                }
            }
        }
    }


    VkCommandBuffer RenderSystem::beginFrame() {
        assert(!frameInProgress && "Cant begin frame when it is has already started.");
        VkResult result = swapchain->acquireNextImage(&imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapchain(); return VK_NULL_HANDLE;
        }
        frameInProgress = true;
        if ((result != VK_SUCCESS) && (result != VK_SUBOPTIMAL_KHR))
            throw std::runtime_error("Failed to aquire next swapchain image!");
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        swapchain->waitForCommandBuffer();
        VkCommandBuffer commandBuffer = commandBuffers[swapchain->currentFrame];
        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
            throw std::runtime_error("Failed to begin recording command buffer!");
        return commandBuffer;
    }
    void RenderSystem::render(FrameInfo& frameInfo) {
        for (size_t i = 0; i < config.passes.size(); i++) {
            size_t numSubPasses = config.passes[i].size();
            for (size_t j = 0; j < numSubPasses; j++) {
                std::vector<VkDescriptorSet> descriptorSets{};
                std::vector<unsigned int>& sampledInputIndices = config.passes[i][j].config.sampledInputIndices;
                size_t numSampledInputs = sampledInputIndices.size();
                if (numSampledInputs > 0)
                    for (size_t k = 0; k < numSampledInputs; k++) {
                        size_t texIndex = sampledInputIndices[k]-1ull;
                        VkImageAspectFlags aspect = swapchain->textures[texIndex][imageIndex]->aspect;
                        if (aspect == VK_IMAGE_ASPECT_DEPTH_BIT)
                            swapchain->textures[texIndex][imageIndex]->transitionLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, frameInfo.commandBuffer);
                        else
                            swapchain->textures[texIndex][imageIndex]->transitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, frameInfo.commandBuffer);
                    }
            }
            beginRenderPass(frameInfo.commandBuffer, i);
            for (size_t j = 0; j < numSubPasses; j++) {
                std::vector<VkDescriptorSet> descriptorSets{};
                if (config.passes[i][j].config.sampledInputIndices.size() > 0) descriptorSets.push_back(sampledInputDescriptorSets[i][j][imageIndex]);
                if (config.passes[i][j].config.inputAttachmentIndices.size() > 0) descriptorSets.push_back(inputAttachmentDescriptorSets[i][j][imageIndex]);
                for (size_t k = 0; k < config.passes[i][j].renderers.size(); k++) {
                    if (descriptorSets.size() > 0)
                        vkCmdBindDescriptorSets(
                            frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, config.passes[i][j].renderers[k]->pipelineLayout,
                            1, static_cast<unsigned int>(descriptorSets.size()), descriptorSets.data(), 0, nullptr
                        );
                    config.passes[i][j].renderers[k]->render(frameInfo);
                }
                if (j != (numSubPasses-1)) nextSubPass(frameInfo.commandBuffer);
            }
            endRenderPass(frameInfo.commandBuffer);
        }
    }
    void RenderSystem::endFrame() {
        assert(frameInProgress && "Cant end frame when it is has not started.");
        VkCommandBuffer commandBuffer = commandBuffers[swapchain->currentFrame];
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
            throw std::runtime_error("Failed to record command buffer!");
        VkResult result = swapchain->submitCommandBuffers(&commandBuffer, &imageIndex);
        if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR) || (window->frameBufferResized)) {
            window->frameBufferResized = false;
            recreateSwapchain();
        } else if (result != VK_SUCCESS)
            throw std::runtime_error("Failed to present next swapchain image!");
        frameInProgress = false;
    }
};