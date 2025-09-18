#include "Swapchain.h"

namespace Eng {
    Swapchain::Swapchain(Device* _device, VkExtent2D extent)
        : device(_device), windowExtent{extent}, oldSwapchain(nullptr)
    {
        init();
    }
    Swapchain::Swapchain(Device* _device, VkExtent2D extent, Swapchain* previousSwapchain)
        : device(_device), windowExtent{extent}, oldSwapchain(previousSwapchain)
    {
        init();
        previousSwapchain = nullptr;
    }
    void Swapchain::init() {
        createSwapChain();
        createRenderPass();
        createTextures();
        createFramebuffers();
        createSyncObjects();
    }

    Swapchain::~Swapchain() {
        for (VkFramebuffer fb : swapChainFramebuffers)
            vkDestroyFramebuffer(device->device, fb, nullptr);
        for (VkImageView imageView : swapChainImageViews)
            vkDestroyImageView(device->device, imageView, nullptr);
        swapChainFramebuffers.clear();
        for (size_t i = 0; i < depthTextures.size(); i++)
            delete depthTextures[i];
        for (size_t i = 0; i < intermediateColorTextures.size(); i++)
            delete intermediateColorTextures[i];
        if (swapChain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(device->device, swapChain, nullptr);
            swapChain = VK_NULL_HANDLE;
        }
        vkDestroyRenderPass(device->device, renderPass, nullptr);
        // rather than exactly MAX_FRAMES_IN_FLIGHT, length of renderFinishedSemaphores
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
        unsigned int imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (
                swapChainSupport.capabilities.maxImageCount > 0
                && imageCount > swapChainSupport.capabilities.maxImageCount
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
#define intermediateFormat VK_FORMAT_B8G8R8A8_UNORM
    void Swapchain::createRenderPass() {
        // attachement 0, temp color buffer
        VkAttachmentDescription intermediateColorAttachment{};
        intermediateColorAttachment.format = intermediateFormat;
        intermediateColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        intermediateColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        intermediateColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        intermediateColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        intermediateColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        intermediateColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        intermediateColorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        // attachement 1, depth buffer
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = findDepthFormat();
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        // attachement 2, color buffer
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // subpass 0
        VkAttachmentReference colorAttachmentRef0{};
        colorAttachmentRef0.attachment = 0;
        colorAttachmentRef0.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;// important/note/idk
        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        VkSubpassDescription subpass0Description{};
        subpass0Description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass0Description.colorAttachmentCount = 1;
        subpass0Description.pColorAttachments = &colorAttachmentRef0;
        subpass0Description.pDepthStencilAttachment = &depthAttachmentRef;
        // dependency 0
        VkSubpassDependency dependency0{};
        dependency0.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency0.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency0.srcAccessMask = 0;
        dependency0.dstSubpass = 0;
        dependency0.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency0.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        // subpass 1
        VkAttachmentReference inputAttachmentRef{};
        inputAttachmentRef.attachment = 0;
        inputAttachmentRef.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        VkAttachmentReference colorAttachmentRef1{};
        colorAttachmentRef1.attachment = 2;
        colorAttachmentRef1.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkSubpassDescription subpass1Description{};
        subpass1Description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass1Description.inputAttachmentCount = 1;
        subpass1Description.pInputAttachments = &inputAttachmentRef;
        subpass1Description.colorAttachmentCount = 1;
        subpass1Description.pColorAttachments = &colorAttachmentRef1;
        // dependency 1
        VkSubpassDependency dependency1{};
        dependency1.srcSubpass = 0;
        dependency1.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency1.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency1.dstSubpass = 1;
        dependency1.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependency1.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        std::array<VkAttachmentDescription, 3> attachments = {intermediateColorAttachment, depthAttachment, colorAttachment};
        renderPassInfo.attachmentCount = static_cast<unsigned int>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        std::array<VkSubpassDescription, 2> subpasses = {subpass0Description, subpass1Description};
        renderPassInfo.subpassCount = static_cast<unsigned int>(subpasses.size());
        renderPassInfo.pSubpasses = subpasses.data();
        std::array<VkSubpassDependency, 2> dependencies = {dependency0, dependency1};
        renderPassInfo.dependencyCount = static_cast<unsigned int>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();
        if (vkCreateRenderPass(device->device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
            throw std::runtime_error("Failed to create render pass!");
    }
    void Swapchain::createTextures() {
        swapChainDepthFormat = findDepthFormat();
        for (int i = 0; i < swapChainImages.size(); i++) {
            intermediateColorTextures.push_back(new Texture(device, swapChainExtent.width, swapChainExtent.height, nullptr, intermediateFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false));
            depthTextures.push_back(new Texture(device, swapChainExtent.width, swapChainExtent.height, nullptr, swapChainDepthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED, false));
        }
        swapChainImageViews.resize(swapChainImages.size());
        for (size_t i = 0; i < swapChainImages.size(); i++) {
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
    }
    void Swapchain::createFramebuffers() {
        swapChainFramebuffers.resize(swapChainImages.size());
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            std::array<VkImageView, 3> attachments = {intermediateColorTextures[i]->getView(), depthTextures[i]->getView(), swapChainImageViews[i]};
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<unsigned int>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;
            if (vkCreateFramebuffer(device->device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create framebuffer!");
        }
    }
    void Swapchain::createSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        // note the difference in mine and Brendan Galea's implementation is the size of renderFinishedSemaphores
        renderFinishedSemaphores.resize(swapChainImages.size());
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);
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
#if defined(_DEBUG) && (_DEBUG==1)
                std::cout << "Present mode: Mailbox" << '\n';
#endif
                return availablePresentMode;
            }
        }
#endif
#if defined(_DEBUG) && (_DEBUG==1)
        std::cout << "Present mode: V-Sync" << '\n';
#endif
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