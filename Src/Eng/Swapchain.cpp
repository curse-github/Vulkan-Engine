#include "Swapchain.h"

namespace Eng {
    Swapchain::Swapchain(Device* _device, VkExtent2D extent, const std::vector<std::vector<SubPassConfig>>& _passConfigs, const std::vector<TextureConfig>& _textureConfigs)
        : device(_device), windowExtent{extent}, passConfigs(_passConfigs), oldSwapchain(nullptr)
    {
        init(_textureConfigs);
    }
    Swapchain::Swapchain(Device* _device, VkExtent2D extent, const std::vector<std::vector<SubPassConfig>>& _passConfigs, const std::vector<TextureConfig>& _textureConfigs, Swapchain* previousSwapchain)
        : device(_device), windowExtent{extent}, passConfigs(_passConfigs), oldSwapchain(previousSwapchain)
    {
        init(_textureConfigs);
        previousSwapchain = nullptr;
    }
    void Swapchain::init(const std::vector<TextureConfig>& _textureConfigs) {
        textureConfigs.push_back(TextureConfig::createDepthTexture());
        if (_textureConfigs.size() > 0)
            textureConfigs.insert(textureConfigs.end(), _textureConfigs.begin(), _textureConfigs.end());
        
        createSwapChain();
        createTextures();
        renderPasses.resize(passConfigs.size(), VK_NULL_HANDLE);
        for (size_t i = 0; i < renderPasses.size(); i++) {
            createRenderPass(i);
            createRenderPassFrameBuffers(i);
        }
        createSyncObjects();
    }

    Swapchain::~Swapchain() {
        for (VkFramebuffer fb : swapChainFramebuffers)
            vkDestroyFramebuffer(device->device, fb, nullptr);
        for (VkImageView imageView : swapChainImageViews)
            vkDestroyImageView(device->device, imageView, nullptr);
        swapChainFramebuffers.clear();
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
        if (vkCreateSwapchainKHR(device->device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
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
    void Swapchain::createTextures() {
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
        // get what textures are being used as inputs
        for (size_t i = 0; i < passConfigs.size(); i++) {
            for (size_t j = 0; j < passConfigs[i].size(); j++) {
                std::vector<unsigned int>& inputAttachmentIndexes = passConfigs[i][j].inputAttachmentIndexes;
                for (size_t k = 0; k < inputAttachmentIndexes.size(); k++) {
                    unsigned int l = inputAttachmentIndexes[k];
                    if (l != 0) textureConfigs[l-1ull].usage = (VkImageUsageFlagBits)(textureConfigs[l-1ull].usage|VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
                }
            }
        }
        // create main depth texture
        swapChainDepthFormat = findDepthFormat();
        textureConfigs[0].format = swapChainDepthFormat;// this texture is inserted automatically specifically to be depth texture
        // create all other textures and image views
        textureDescriptors.resize(numTextures);
        for (size_t i = 0; i < numTextures; i++) {
            textures[i].resize(imageCount);
            textureDescriptors[i].resize(imageCount);
            // create texture
            for (size_t j = 0; j < imageCount; j++) {
                textures[i][j] = new Texture(
                    device, swapChainExtent.width, swapChainExtent.height, nullptr, textureConfigs[i].format, VK_IMAGE_TILING_OPTIMAL,
                    textureConfigs[i].usage, textureConfigs[i].aspect, textureConfigs[i].imageLayout, textureConfigs[i].createSampler
                );
                textureDescriptors[i][j] = textures[i][j]->descriptorInfo();
            }
        }
    }
    void Swapchain::createRenderPass(const unsigned int& renderPassIndex) {
        std::vector<VkAttachmentDescription> attachments{};
        // main swapchain color image clear color and attachment description
        clearValues.push_back(VkClearValue{ color:{0.0f, 0.0f, 0.0f, 1.0f} });
        attachments.push_back(VkAttachmentDescription{
            0,// flags
            swapChainImageFormat,
            VK_SAMPLE_COUNT_1_BIT,
            VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_STORE,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        });
        // other attachments
        size_t numTextures = textureConfigs.size();
        for (size_t i = 0; i < numTextures; i++) {
            // create clear color for texture
            if (textureConfigs[i].aspect == VK_IMAGE_ASPECT_DEPTH_BIT)
                clearValues.push_back(VkClearValue{ depthStencil:{1.0f, 0u} });
            else clearValues.push_back(VkClearValue{ color:{0.0f, 0.0f, 0.0f, 1.0f} });
            // create attachment description
            attachments.push_back(VkAttachmentDescription{
                0,// flags
                textureConfigs[i].format,
                VK_SAMPLE_COUNT_1_BIT,
                VK_ATTACHMENT_LOAD_OP_CLEAR,
                VK_ATTACHMENT_STORE_OP_STORE,
                VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_IMAGE_LAYOUT_UNDEFINED,
                textureConfigs[i].attachmentLayout
            });
        }
        std::vector<SubPassConfig> subpassConfigs = passConfigs[renderPassIndex];
        std::vector<VkAttachmentReference> attachmentReferences{};
        std::vector<VkSubpassDependency> dependencies{};
        std::vector<unsigned int> attachmentsLastWrittenToBy(attachments.size(), VK_SUBPASS_EXTERNAL);
        for (size_t i = 0; i < subpassConfigs.size(); i++) {
            // setup input attachment references
            size_t inputAttachmentCount = subpassConfigs[i].inputAttachmentIndexes.size();
            for (size_t j = 0; j < inputAttachmentCount; j++) {
                unsigned int attachmentIndex = subpassConfigs[i].inputAttachmentIndexes[j];
                assert((attachmentIndex != 0) && "Cannot input color output attachment as an input.");
                attachmentReferences.push_back({attachmentIndex, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
                unsigned int lastWrittenBy = attachmentsLastWrittenToBy[attachmentIndex];
                // resolve dependency
                if ((lastWrittenBy != VK_SUBPASS_EXTERNAL))
                    dependencies.push_back(VkSubpassDependency{
                        lastWrittenBy, static_cast<unsigned int>(i),
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,// what stages of the src must wait before giving it to the dst
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,// what stages of the dst must wait for it to be ready from the src
                        (unsigned int)((lastWrittenBy == VK_SUBPASS_EXTERNAL) ? 0 : VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT),// what kind of acessing the src can do
                        VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,// what kind of acessing the dst can do
                        VK_DEPENDENCY_BY_REGION_BIT
                    });
            }
            // setup output color attachment references
            size_t colorAttachmentCount = subpassConfigs[i].colorAttachmentIndexes.size();
            for (size_t j = 0; j < colorAttachmentCount; j++) {
                unsigned int attachmentIndex = subpassConfigs[i].colorAttachmentIndexes[j];
                assert((attachments[attachmentIndex].finalLayout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) && "Cannot set depth texture attachment as an color output.");
                attachmentReferences.push_back({attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
                // resolve dependency
                unsigned int lastWrittenBy = attachmentsLastWrittenToBy[attachmentIndex];
                if ((attachmentIndex == 0) || (lastWrittenBy != VK_SUBPASS_EXTERNAL))
                    dependencies.push_back(VkSubpassDependency{
                        lastWrittenBy, static_cast<unsigned int>(i),
                        (unsigned int)((lastWrittenBy == VK_SUBPASS_EXTERNAL) ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT),// what stages of the src must wait before giving it to the dst
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,// what stages of the dst must wait for it to be ready from the src
                        (unsigned int)((lastWrittenBy == VK_SUBPASS_EXTERNAL) ? 0 : VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT),// what kind of acessing the src can do
                        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,// what kind of acessing the dst can do
                        VK_DEPENDENCY_BY_REGION_BIT
                    });
                attachmentsLastWrittenToBy[attachmentIndex] = static_cast<unsigned int>(i);
            }
            // setup depth texture attachment reference(if any)
            if (subpassConfigs[i].depthAttachmentIndex != SubPassConfig::NO_DEPTH_ATTACHMENT) {
                unsigned int attachmentIndex = subpassConfigs[i].depthAttachmentIndex;
                assert((attachments[attachmentIndex].finalLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) && "Cannot set non depth texture attachment as a sub-passes depth texture.");
                attachmentReferences.push_back({attachmentIndex, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL});
                // resolve dependency
                unsigned int lastWrittenBy = attachmentsLastWrittenToBy[attachmentIndex];
                dependencies.push_back(VkSubpassDependency{
                    lastWrittenBy, static_cast<unsigned int>(i),
                    (unsigned int)((lastWrittenBy == VK_SUBPASS_EXTERNAL) ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : (VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT)),// what stages of the src must wait before giving it to the dst
                    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,// what stages of the dst must wait for it to be ready from the src
                    (unsigned int)((lastWrittenBy == VK_SUBPASS_EXTERNAL) ? 0 : VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT),// what kind of acessing the src can do
                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,// what kind of acessing the dst can do
                    VK_DEPENDENCY_BY_REGION_BIT
                });
                attachmentsLastWrittenToBy[attachmentIndex] = static_cast<unsigned int>(i);
            }
        }
        size_t attachmentIndex = 0ull;
        std::vector<VkSubpassDescription> subpasses{};
        for (size_t i = 0; i < subpassConfigs.size(); i++) {
            // get pointer to input attachment references
            size_t inputAttachmentCount = subpassConfigs[i].inputAttachmentIndexes.size();
            VkAttachmentReference* pInputAttachments = nullptr;
            if (inputAttachmentCount > 0) pInputAttachments = attachmentReferences.data() + attachmentIndex;
            attachmentIndex += inputAttachmentCount;
            // setup output color attachments
            size_t colorAttachmentCount = subpassConfigs[i].colorAttachmentIndexes.size();
            VkAttachmentReference* pColorAttachments = nullptr;
            if (colorAttachmentCount > 0) pColorAttachments = attachmentReferences.data() + attachmentIndex;
            attachmentIndex += colorAttachmentCount;
            // setup depth texture attachment(if any)
            VkAttachmentReference* pDepthAttachment = nullptr;
            if (subpassConfigs[i].depthAttachmentIndex != SubPassConfig::NO_DEPTH_ATTACHMENT) {
                pDepthAttachment = attachmentReferences.data() + attachmentIndex;
                attachmentIndex++;
            }
            // create subpass description
            subpasses.push_back(VkSubpassDescription{
                0, VK_PIPELINE_BIND_POINT_GRAPHICS,
                static_cast<unsigned int>(inputAttachmentCount), pInputAttachments,
                static_cast<unsigned int>(colorAttachmentCount), pColorAttachments,
                nullptr,
                pDepthAttachment,
                0, nullptr
            });
        }

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<unsigned int>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = static_cast<unsigned int>(subpasses.size());
        renderPassInfo.pSubpasses = subpasses.data();
        renderPassInfo.dependencyCount = static_cast<unsigned int>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();
        if (vkCreateRenderPass(device->device, &renderPassInfo, nullptr, &renderPasses[renderPassIndex]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create render pass!");
    }
    void Swapchain::createRenderPassFrameBuffers(const unsigned int& renderPassIndex) {
        swapChainFramebuffers.resize(imageCount);
        for (size_t i = 0; i < imageCount; i++) {
            std::vector<VkImageView> attachmentViews{};
            attachmentViews.push_back(swapChainImageViews[i]);
            for (size_t j = 0; j < textures.size(); j++)
                attachmentViews.push_back(textures[j][i]->getView());
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPasses[renderPassIndex];
            framebufferInfo.attachmentCount = static_cast<unsigned int>(attachmentViews.size());
            framebufferInfo.pAttachments = attachmentViews.data();
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;
            if (vkCreateFramebuffer(device->device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
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