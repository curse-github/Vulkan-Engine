#include "Swapchain.h"

namespace Eng {
    Swapchain::Swapchain(Device* _device, VkExtent2D extent, const std::vector<Texture::Config>& textureConfigs, const std::vector<RenderPassConfig>& passConfigs)
        : device(_device), windowExtent(extent), oldSwapchain(nullptr)
    {
        init(textureConfigs, passConfigs);
    }
    Swapchain::Swapchain(Device* _device, VkExtent2D extent, const std::vector<Texture::Config>& textureConfigs, const std::vector<RenderPassConfig>& passConfigs, Swapchain* previousSwapchain)
        : device(_device), windowExtent(extent), oldSwapchain(previousSwapchain)
    {
        init(textureConfigs, passConfigs);
        previousSwapchain = nullptr;
    }
    void Swapchain::init(const std::vector<Texture::Config>& textureConfigs, const std::vector<RenderPassConfig>& _passConfigs) {
        createSwapChain();
        swapChainDepthFormat = findDepthFormat();
        std::vector<RenderPassConfig> passConfigs = _passConfigs;
        createTextures(textureConfigs, passConfigs);
        renderPasses.resize(passConfigs.size(), VK_NULL_HANDLE);
        swapChainFramebuffers.resize(passConfigs.size());
        for (size_t i = 0; i < renderPasses.size(); i++)
            createRenderPass(i, passConfigs[i]);
        createSyncObjects();
    }

    Swapchain::~Swapchain() {
        for (size_t i = 0; i < swapChainFramebuffers.size(); i++)
            for (size_t j = 0; j < swapChainFramebuffers[i].size(); j++)
                vkDestroyFramebuffer(device->device, swapChainFramebuffers[i][j], nullptr);
        swapChainFramebuffers.clear();
        for (VkImageView imageView : swapChainImageViews)
            vkDestroyImageView(device->device, imageView, nullptr);
        for (size_t i = 0; i < textures.size(); i++)
            for (size_t j = 0; j < imageCount; j++)
                delete textures[i][j];
        if (swapChain != VK_NULL_HANDLE)
            vkDestroySwapchainKHR(device->device, swapChain, nullptr);
        for (size_t i = 0; i < renderPasses.size(); i++)
            vkDestroyRenderPass(device->device, renderPasses[i], nullptr);
        for (VkSemaphore s : renderFinishedSemaphores)
            vkDestroySemaphore(device->device, s, nullptr);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(device->device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(device->device, inFlightFences[i], nullptr);
        }
    }




    void Swapchain::createSwapChain() {
        SwapChainSupportDetails swapChainSupport = device->getSwapChainSupport();
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
        imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (
            (swapChainSupport.capabilities.maxImageCount > 0) &&
            (imageCount > swapChainSupport.capabilities.maxImageCount)
        ) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = device->surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        QueueFamilyIndices indices = device->findPhysicalQueueFamilies();
        unsigned int queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};
        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;      // Optional
            createInfo.pQueueFamilyIndices = nullptr;  // Optional
        }
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = (oldSwapchain == nullptr) ? VK_NULL_HANDLE : oldSwapchain->swapChain;
        VkResult res = vkCreateSwapchainKHR(device->device, &createInfo, nullptr, &swapChain);
        std::cout << "res = " << string_VkResult(res) << '\n';
        if (res != VK_SUCCESS)
            throw std::runtime_error("Failed to create swap chain!");
        // we only specified a minimum number of images in the swap chain, so the implementation is
        // allowed to create a swap chain with more. That's why we'll first query the final number of
        // images with vkGetSwapchainImagesKHR, then resize the container and finally call it again to
        // retrieve the handles.
        vkGetSwapchainImagesKHR(device->device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device->device, swapChain, &imageCount, swapChainImages.data());
        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }
    void Swapchain::createTextures(const std::vector<Texture::Config>& textureConfigs, std::vector<RenderPassConfig>& passConfigs) {
        // create swapchain image views
        swapChainImageViews.resize(imageCount);
        for (size_t i = 0; i < imageCount; i++) {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = swapChainImages[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = swapChainImageFormat;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;
            if (vkCreateImageView(device->device, &viewInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create texture image view!");
        }
        size_t numTextures = textureConfigs.size();
        textures.resize(numTextures);
        // create all other textures and image views
        textureDescriptors.resize(numTextures);
        for (size_t i = 0; i < numTextures; i++) {
            textures[i].resize(imageCount);
            textureDescriptors[i].resize(imageCount);
            // create texture
            for (size_t j = 0; j < imageCount; j++) {
                VkFormat format = ((textureConfigs[i].aspect == VK_IMAGE_ASPECT_DEPTH_BIT) ? swapChainDepthFormat : swapChainImageFormat);
                textures[i][j] = new Texture(device, swapChainExtent.width, swapChainExtent.height, nullptr, format, VK_IMAGE_TILING_OPTIMAL, textureConfigs[i]);
                textureDescriptors[i][j] = textures[i][j]->descriptorInfo();
            }
        }
        for (size_t i = 0; i < passConfigs.size(); i++) {
            for (size_t j = 0; j < passConfigs[i].attachments.size(); j++) {
                if (textureConfigs[passConfigs[i].attachmentIndexToTextureIndex[j]-1ull].aspect == VK_IMAGE_ASPECT_DEPTH_BIT)
                    passConfigs[i].attachments[j].format = swapChainDepthFormat;
                else
                    passConfigs[i].attachments[j].format = swapChainImageFormat;
            }
        }
    }
    void Swapchain::createRenderPass(const unsigned int& renderPassIndex, const RenderPassConfig& passConfig) {
        std::vector<VkSubpassDescription> subpasses{};
        const std::vector<SubPassConfig>& subpassConfigs = passConfig.subPassConfigs;
        for (size_t subPassIndex = 0; subPassIndex < subpassConfigs.size(); subPassIndex++) {
            // get pointer to input attachment references
            unsigned int inputAttachmentCount = static_cast<unsigned int>(subpassConfigs[subPassIndex].inputAttachmentReferences.size());
            const VkAttachmentReference* pInputAttachments = nullptr;
            if (inputAttachmentCount > 0)
                pInputAttachments = subpassConfigs[subPassIndex].inputAttachmentReferences.data();
            // setup output color attachments
            unsigned int colorAttachmentCount = static_cast<unsigned int>(subpassConfigs[subPassIndex].colorAttachmentReferences.size());
            const VkAttachmentReference* pColorAttachments = nullptr;
            if (colorAttachmentCount > 0)
                pColorAttachments = subpassConfigs[subPassIndex].colorAttachmentReferences.data();
            // setup depth texture attachment(if any)
            const VkAttachmentReference* pDepthAttachment = nullptr;
            if (subpassConfigs[subPassIndex].hasDepthAttachment)
                pDepthAttachment = &subpassConfigs[subPassIndex].depthAttachmentReferences;
            // create subpass description
            subpasses.push_back(VkSubpassDescription{
                0, VK_PIPELINE_BIND_POINT_GRAPHICS,
                inputAttachmentCount, pInputAttachments,
                colorAttachmentCount, pColorAttachments,
                nullptr,
                pDepthAttachment,
                0, nullptr
            });
        }

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<unsigned int>(passConfig.attachments.size());
        renderPassInfo.pAttachments = passConfig.attachments.data();
        renderPassInfo.subpassCount = static_cast<unsigned int>(subpasses.size());
        renderPassInfo.pSubpasses = subpasses.data();
        renderPassInfo.dependencyCount = static_cast<unsigned int>(passConfig.dependencies.size());
        renderPassInfo.pDependencies = passConfig.dependencies.data();
        if (vkCreateRenderPass(device->device, &renderPassInfo, nullptr, &renderPasses[renderPassIndex]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create render pass!");
        createRenderPassFrameBuffers(renderPassIndex, passConfig);
    }
    void Swapchain::createRenderPassFrameBuffers(const unsigned int& renderPassIndex, const RenderPassConfig& passConfig) {
        swapChainFramebuffers[renderPassIndex].resize(imageCount);
        for (size_t i = 0; i < imageCount; i++) {
            std::vector<VkImageView> attachmentViews{};
            for (const size_t& textureIndex : passConfig.attachmentIndexToTextureIndex) {
                if (textureIndex == 0)
                    attachmentViews.push_back(swapChainImageViews[i]);
                else
                    attachmentViews.push_back(textures[textureIndex-1ull][i]->getView());
            }
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPasses[renderPassIndex];
            framebufferInfo.attachmentCount = static_cast<unsigned int>(attachmentViews.size());
            framebufferInfo.pAttachments = attachmentViews.data();
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;
            if (vkCreateFramebuffer(device->device, &framebufferInfo, nullptr, &swapChainFramebuffers[renderPassIndex][i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create framebuffer!");
        }
    }
    void Swapchain::createSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);
        renderFinishedSemaphores.resize(imageCount, VK_NULL_HANDLE);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);
        imagesInFlight.resize(imageCount, VK_NULL_HANDLE);
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (
                    vkCreateSemaphore(device->device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS
                    || vkCreateFence(device->device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS
                )
                throw std::runtime_error("Failed to create per-frame sync objects");
        }
        for (size_t i = 0; i < renderFinishedSemaphores.size(); ++i) {
            if (vkCreateSemaphore(device->device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create per-image renderFinished semaphore");
        }
    }




    VkSurfaceFormatKHR Swapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const VkSurfaceFormatKHR& availableFormat : availableFormats) {
            if (
                    availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM// I only have variations of unorm, snorm, or srgb 
                    && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR// the only color space i have avaliable
                ) {
                return availableFormat;
            }
        }
#if defined(_DEBUG) && (_DEBUG==1)
        std::cout << "Unable to get desired SurfaceFormat.\n";
#endif
        return availableFormats[0];
    }
    VkPresentModeKHR Swapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        // https://registry.khronos.org/vulkan/specs/latest/man/html/VkPresentModeKHR.html
#ifndef VSYNC
        // this picks the mailbox type by default and v-sync as the fallback if that does not work
        for (const VkPresentModeKHR& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }
#endif
        std::cout << "V-Sync is enabled.\n";
        return VK_PRESENT_MODE_FIFO_KHR;
    }
    VkExtent2D Swapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<unsigned int>::max())
            return capabilities.currentExtent;
        else {
            VkExtent2D actualExtent = windowExtent;
            actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
            return actualExtent;
        }
    }
    VkFormat Swapchain::findDepthFormat() {
        return device->findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }




    VkResult Swapchain::acquireNextImage(unsigned int* imageIndex) {
        vkWaitForFences(device->device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
        // imageAvailableSemaphores[currentFrame] must be a not signaled semaphore
        VkResult result = vkAcquireNextImageKHR(device->device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, imageIndex);
        return result;
    }
    void Swapchain::waitForCommandBuffer() {
        vkWaitForFences(device->device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    }
    VkResult Swapchain::submitCommandBuffers(const VkCommandBuffer* buffers, unsigned int* imageIndex) {
        if (imagesInFlight[*imageIndex] != VK_NULL_HANDLE)
            vkWaitForFences(device->device, 1, &imagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
        imagesInFlight[*imageIndex] = inFlightFences[currentFrame];
        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[*imageIndex]};
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = buffers;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
        vkResetFences(device->device, 1, &inFlightFences[currentFrame]);
        if (vkQueueSubmit(device->graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
            throw std::runtime_error("Failed to submit draw command buffer!");
        VkSwapchainKHR swapChains[] = {swapChain};
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = imageIndex;
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        return vkQueuePresentKHR(device->presentQueue, &presentInfo);
    }
    bool Swapchain::swapchainsCompatible(const Swapchain& otherSwapchain) const {
        return (otherSwapchain.swapChainDepthFormat == swapChainDepthFormat) && (otherSwapchain.swapChainImageFormat == swapChainImageFormat);
    }
};