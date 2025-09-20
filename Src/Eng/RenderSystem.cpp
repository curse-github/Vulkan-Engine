#include "RenderSystem.h"
#include "Renderers.h"

namespace Eng {
    RenderSystem::RenderSystem(Window* _window, Device* _device)
        : window(_window), device(_device), swapchain(nullptr)
    {
        recreateSwapchain();
        createCommandBuffers();
        // create uniforms used by pipelines
        inputAttachmentDescriptorSetLayout = DescriptorSetLayout::Builder(device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT, 1).build();
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
        Swapchain::SwapchainConfig config(
            {// texture attachment configs
                Swapchain::TextureConfig::createColorImage()// index 2
            },
            
            std::vector<std::vector<Swapchain::SubPassConfig>>{{{// render pass list, sub-pass config list, sub-pass config constructor
                // input texture, output color, and depth texture attachment indexes, respectively
                {}, {2}, 1
            }, {
                {2}, {0}, Swapchain::SubPassConfig::NO_DEPTH_ATTACHMENT
            }}}
        );
        if (swapchain == nullptr) swapchain = new Swapchain(device, VkExtent2D{static_cast<unsigned int>(window->size.x), static_cast<unsigned int>(window->size.y)}, config);
        else {
            Swapchain* oldSwapchain = swapchain;
            swapchain = new Swapchain(device, VkExtent2D{static_cast<unsigned int>(window->size.x), static_cast<unsigned int>(window->size.y)}, config, oldSwapchain);
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
    void RenderSystem::freeCommandBuffers() {
        vkFreeCommandBuffers(device->device, device->commandPool, static_cast<unsigned int>(commandBuffers.size()), commandBuffers.data());
        commandBuffers.clear();
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

    void RenderSystem::allocateInputAttachments(DescriptorPool* _globalDescriptorPool) {
        globalDescriptorPool = _globalDescriptorPool;
        inputAttachmentDescriptorSets.resize(swapchain->getImageCount());
        for (size_t i = 0; i < swapchain->getImageCount(); i++) {
            DescriptorWriter(inputAttachmentDescriptorSetLayout, globalDescriptorPool)
                .writeImage(0, &swapchain->textureDescriptors[1][i]).build(inputAttachmentDescriptorSets[i]);
        }
    }
    void RenderSystem::overwriteInputAttachments() {
        inputAttachmentDescriptorSets.resize(swapchain->getImageCount());
        for (size_t i = 0; i < swapchain->getImageCount(); i++) {
            DescriptorWriter(inputAttachmentDescriptorSetLayout, globalDescriptorPool)
                .writeImage(0, &swapchain->textureDescriptors[1][i]).overwrite(inputAttachmentDescriptorSets[i]);
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
        VkCommandBuffer commandBuffer = getCurrentCommandBuffer();
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        swapchain->waitForCommandBuffer();
        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
            throw std::runtime_error("Failed to begin recording command buffer!");
        return commandBuffer;
    }
    void RenderSystem::beginRenderPass(VkCommandBuffer commandBuffer, const unsigned int& _renderPassIndex) {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = swapchain->renderPasses[_renderPassIndex];
        renderPassInfo.framebuffer = swapchain->swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = {swapchain->swapChainExtent.width, swapchain->swapChainExtent.height};
        renderPassInfo.clearValueCount = static_cast<unsigned int>(swapchain->clearValues.size());
        renderPassInfo.pClearValues = swapchain->clearValues.data();
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
    void RenderSystem::endFrame() {
        assert(frameInProgress && "Cant end frame when it is has not started.");
        VkCommandBuffer commandBuffer = getCurrentCommandBuffer();
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