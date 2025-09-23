#include "Swapchain.h"

namespace Eng {
    Swapchain::Swapchain(Device* _device, VkExtent2D extent, const std::vector<std::vector<SubPassConfig>>& _passConfigs)
        : device(_device), windowExtent{extent}, passConfigs(_passConfigs), oldSwapchain(nullptr)
    {
        init();
    }
    Swapchain::Swapchain(Device* _device, VkExtent2D extent, const std::vector<std::vector<SubPassConfig>>& _passConfigs, Swapchain* previousSwapchain)
        : device(_device), windowExtent{extent}, passConfigs(_passConfigs), oldSwapchain(previousSwapchain)
    {
        init();
        previousSwapchain = nullptr;
    }
    void Swapchain::init() {
        createSwapChain();
        swapChainDepthFormat = findDepthFormat();
        std::vector<TextureConfig> textureConfigs = processConfig();
        createTextures(textureConfigs);
        renderPasses.resize(passConfigs.size(), VK_NULL_HANDLE);
        swapChainFramebuffers.resize(passConfigs.size());
        for (size_t i = 0; i < renderPasses.size(); i++) {
            createRenderPass(i);
            createRenderPassFrameBuffers(i);
        }
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

    std::vector<Swapchain::TextureConfig> Swapchain::processConfig() {
        std::cout << "process config\n";
        size_t texturesNeeded = 2;
        std::vector<TextureConfig> textureConfigs(texturesNeeded);
        std::vector<bool> textureIsWritten(texturesNeeded);
        std::vector<bool> textureIsRead(texturesNeeded);
        
        size_t numRenderPasses = passConfigs.size();
        clearValues.resize(numRenderPasses);
        attachments.resize(numRenderPasses);
        inputAttachmentReferences.resize(numRenderPasses);
        colorAttachmentReferences.resize(numRenderPasses);
        depthAttachmentReferences.resize(numRenderPasses);
        dependencies.resize(numRenderPasses);
        attachmentIndexToTextureIndex.resize(numRenderPasses);
        std::cout << "render passes\n";
        for (size_t renderPassIndex = 0; renderPassIndex < numRenderPasses; renderPassIndex++) {
            std::cout << "    render pass #" << renderPassIndex << '\n';
            std::vector<SubPassConfig>& subpassConfigs = passConfigs[renderPassIndex];
            size_t numSubPasses = subpassConfigs.size();
            std::vector<unsigned int> lastUsedByThisRenderPass(texturesNeeded, VK_SUBPASS_EXTERNAL);
            for (size_t subPassIndex = 0; subPassIndex < numSubPasses; subPassIndex++) {
                std::cout << "        sub pass #" << subPassIndex << ", pass one\n";
                SubPassConfig& subPass = subpassConfigs[subPassIndex];
                std::vector<unsigned int>& sampledInputIndices = subPass.sampledInputIndices;
                size_t numSampledInputs = sampledInputIndices.size();
                for (size_t sampledInputIndex = 0; sampledInputIndex < numSampledInputs; sampledInputIndex++) {
                    std::cout << "            sampled input #" << sampledInputIndex << '\n';
                    unsigned int& textureIndex = sampledInputIndices[sampledInputIndex];
                    if (textureIndex >= texturesNeeded) {
                        texturesNeeded = textureIndex+1;
                        textureConfigs.resize(texturesNeeded);
                        textureIsWritten.resize(texturesNeeded, false);
                        textureIsRead.resize(texturesNeeded, false);
                        lastUsedByThisRenderPass.resize(texturesNeeded, VK_SUBPASS_EXTERNAL);
                    }
                    textureConfigs[textureIndex].usage = textureConfigs[textureIndex].usage|VK_IMAGE_USAGE_SAMPLED_BIT;
                    textureConfigs[textureIndex].createSampler = true;
                    if (lastUsedByThisRenderPass[textureIndex] == subPassIndex) throw std::runtime_error("Cannot use texture more than once in a subpass.");
                }

                std::vector<unsigned int>& inputAttachmentIndices = subPass.inputAttachmentIndices;
                size_t numInputAttachments = inputAttachmentIndices.size();
                for (size_t inputAttachmentIndex = 0; inputAttachmentIndex < numInputAttachments; inputAttachmentIndex++) {
                    std::cout << "            input attachment #" << inputAttachmentIndex << '\n';
                    unsigned int& textureIndex = inputAttachmentIndices[inputAttachmentIndex];
                    if (textureIndex >= texturesNeeded) {
                        texturesNeeded = textureIndex+1;
                        textureConfigs.resize(texturesNeeded);
                        textureIsWritten.resize(texturesNeeded, false);
                        textureIsRead.resize(texturesNeeded, false);
                        lastUsedByThisRenderPass.resize(texturesNeeded, VK_SUBPASS_EXTERNAL);
                    }
                    textureConfigs[textureIndex].usage = textureConfigs[textureIndex].usage|VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
                    if (lastUsedByThisRenderPass[textureIndex] == subPassIndex) throw std::runtime_error("Cannot use texture more than once in a subpass.");
                    lastUsedByThisRenderPass[textureIndex] = subPassIndex;
                }

                std::vector<unsigned int>& colorAttachmentIndices = subPass.colorAttachmentIndices;
                size_t numColorAttachments = colorAttachmentIndices.size();
                for (size_t colorAttachmentIndex = 0; colorAttachmentIndex < numColorAttachments; colorAttachmentIndex++) {
                    std::cout << "            color attachment #" << colorAttachmentIndex << '\n';
                    unsigned int& textureIndex = colorAttachmentIndices[colorAttachmentIndex];
                    if (textureIndex >= texturesNeeded) {
                        texturesNeeded = textureIndex+1;
                        textureConfigs.resize(texturesNeeded);
                        textureIsWritten.resize(texturesNeeded, false);
                        textureIsRead.resize(texturesNeeded, false);
                        lastUsedByThisRenderPass.resize(texturesNeeded, VK_SUBPASS_EXTERNAL);
                    }
                    textureConfigs[textureIndex] = TextureConfig::createColorImage(swapChainImageFormat);
                    if (lastUsedByThisRenderPass[textureIndex] == subPassIndex) throw std::runtime_error("Cannot use texture more than once in a subpass.");
                    lastUsedByThisRenderPass[textureIndex] = subPassIndex;
                }

                bool hasDepthAttachment = (subPass.depthAttachmentIndex != SubPassConfig::NO_DEPTH_ATTACHMENT);
                if (hasDepthAttachment) {
                    std::cout << "            depth attachment\n";
                    unsigned int& textureIndex = subPass.depthAttachmentIndex;
                    if (textureIndex >= texturesNeeded) {
                        texturesNeeded = textureIndex+1;
                        textureConfigs.resize(texturesNeeded);
                        textureIsWritten.resize(texturesNeeded, false);
                        textureIsRead.resize(texturesNeeded, false);
                        lastUsedByThisRenderPass.resize(texturesNeeded, VK_SUBPASS_EXTERNAL);
                    }
                    textureConfigs[textureIndex] = TextureConfig::createDepthTexture(swapChainDepthFormat);
                    if (lastUsedByThisRenderPass[textureIndex] == subPassIndex) throw std::runtime_error("Cannot use texture more than once in a subpass.");
                    lastUsedByThisRenderPass[textureIndex] = subPassIndex;
                }
                std::cout << "        end sub pass #" << subPassIndex << ", pass one\n";
            }
            std::vector<unsigned int> textureIndexToAttachmentIndex(texturesNeeded);
            std::cout << "        texture pass\n";
            for (size_t textureIndex = 0; textureIndex < lastUsedByThisRenderPass.size(); textureIndex++) {
                std::cout << "            texture #" << textureIndex << '\n';
                if (lastUsedByThisRenderPass[textureIndex] == VK_SUBPASS_EXTERNAL) continue;
                std::cout << "                clear value\n";
                if (textureConfigs[textureIndex].aspect == VK_IMAGE_ASPECT_DEPTH_BIT)
                    clearValues[renderPassIndex].push_back(VkClearValue{ depthStencil:{1.0f, 0u} });
                else
                    clearValues[renderPassIndex].push_back(VkClearValue{ color:{0.0f, 0.0f, 0.0f, 1.0f} });
                std::cout << "                attachment\n";
                attachmentIndexToTextureIndex[renderPassIndex].push_back(static_cast<unsigned int>(textureIndex));
                textureIndexToAttachmentIndex[textureIndex] = static_cast<unsigned int>(attachments[renderPassIndex].size());
                if(textureIndex == 0) {
                    attachments[renderPassIndex].push_back(VkAttachmentDescription{
                        0,// flags
                        textureConfigs[textureIndex].format,
                        VK_SAMPLE_COUNT_1_BIT,
                        VK_ATTACHMENT_LOAD_OP_CLEAR,
                        VK_ATTACHMENT_STORE_OP_STORE,
                        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                        VK_ATTACHMENT_STORE_OP_DONT_CARE,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
                    });
                } else
                attachments[renderPassIndex].push_back(VkAttachmentDescription{
                        0,// flags
                        textureConfigs[textureIndex].format,
                        VK_SAMPLE_COUNT_1_BIT,
                        VK_ATTACHMENT_LOAD_OP_CLEAR,
                        VK_ATTACHMENT_STORE_OP_STORE,
                        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                        VK_ATTACHMENT_STORE_OP_DONT_CARE,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        textureConfigs[textureIndex].attachmentLayout
                    });
            }
            std::cout << "        end texture pass\n";

            inputAttachmentReferences[renderPassIndex].resize(numSubPasses);
            colorAttachmentReferences[renderPassIndex].resize(numSubPasses);
            depthAttachmentReferences[renderPassIndex].resize(numSubPasses);
            for (size_t i = 0; i < lastUsedByThisRenderPass.size(); i++) lastUsedByThisRenderPass[i] = VK_SUBPASS_EXTERNAL;
            std::vector<VkImageUsageFlags> lastUsedByAsThisRenderPass(texturesNeeded, 0u);
            for (size_t subPassIndex = 0; subPassIndex < numSubPasses; subPassIndex++) {
                std::cout << "        sub pass #" << subPassIndex << ", pass two\n";
                SubPassConfig& subPass = subpassConfigs[subPassIndex];
                std::vector<unsigned int>& sampledInputIndices = subPass.sampledInputIndices;
                size_t numSampledInputs = sampledInputIndices.size();
                for (size_t sampledInputIndex = 0; sampledInputIndex < numSampledInputs; sampledInputIndex++) {
                    std::cout << "            sampled input #" << sampledInputIndex << '\n';
                    unsigned int& textureIndex = sampledInputIndices[sampledInputIndex];
                    if (!textureIsWritten[textureIndex]) throw std::runtime_error("Cannot read a texture before it has been written to.");
                    textureIsRead[textureIndex] = true;
                    if (lastUsedByThisRenderPass[textureIndex] != VK_SUBPASS_EXTERNAL) throw std::runtime_error("Cannot sample texture written in the same render pass.");
                }

                std::vector<unsigned int>& inputAttachmentIndices = subPass.inputAttachmentIndices;
                size_t numInputAttachments = inputAttachmentIndices.size();
                for (size_t inputAttachmentIndex = 0; inputAttachmentIndex < numInputAttachments; inputAttachmentIndex++) {
                    std::cout << "            intput attachment #" << inputAttachmentIndex << '\n';
                    unsigned int& textureIndex = inputAttachmentIndices[inputAttachmentIndex];
                    if (!textureIsWritten[textureIndex]) throw std::runtime_error("Cannot read a texture before it has been written to.");
                    textureIsRead[textureIndex] = true;
                    inputAttachmentReferences[renderPassIndex][subPassIndex].push_back({textureIndexToAttachmentIndex[textureIndex], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
                    if (lastUsedByThisRenderPass[textureIndex] != VK_SUBPASS_EXTERNAL) {
                        if (lastUsedByAsThisRenderPass[textureIndex] == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
                            dependencies[renderPassIndex].push_back(VkSubpassDependency{
                                lastUsedByThisRenderPass[textureIndex], static_cast<unsigned int>(subPassIndex),
                                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,// what stages of the src must wait before giving it to the dst
                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,// what stages of the dst must wait for it to be ready from the src
                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,// what kind of acessing the src can do
                                VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,// what kind of acessing the dst can do
                                VK_DEPENDENCY_BY_REGION_BIT
                            });
                        else if (lastUsedByAsThisRenderPass[textureIndex] == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                            dependencies[renderPassIndex].push_back(VkSubpassDependency{
                                lastUsedByThisRenderPass[textureIndex], static_cast<unsigned int>(subPassIndex),
                                VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,// what stages of the src must wait before giving it to the dst
                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,// what stages of the dst must wait for it to be ready from the src
                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,// what kind of acessing the src can do
                                VK_ACCESS_INPUT_ATTACHMENT_READ_BIT|VK_ACCESS_SHADER_READ_BIT,// what kind of acessing the dst can do
                                VK_DEPENDENCY_BY_REGION_BIT
                            });
                        } else if (lastUsedByAsThisRenderPass[textureIndex] == VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
                            dependencies[renderPassIndex].push_back(VkSubpassDependency{
                                lastUsedByThisRenderPass[textureIndex], static_cast<unsigned int>(subPassIndex),
                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,// what stages of the src must wait before giving it to the dst
                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,// what stages of the dst must wait for it to be ready from the src
                                VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,// what kind of acessing the src can do
                                VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,// what kind of acessing the dst can do
                                VK_DEPENDENCY_BY_REGION_BIT
                            });
                        else throw std::runtime_error("error!");
                    }
                    lastUsedByThisRenderPass[textureIndex] = subPassIndex;
                    lastUsedByAsThisRenderPass[textureIndex] = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
                }

                std::vector<unsigned int>& colorAttachmentIndices = subPass.colorAttachmentIndices;
                size_t numColorAttachments = colorAttachmentIndices.size();
                for (size_t colorAttachmentIndex = 0; colorAttachmentIndex < numColorAttachments; colorAttachmentIndex++) {
                    std::cout << "            color attachment #" << colorAttachmentIndex << '\n';
                    unsigned int& textureIndex = colorAttachmentIndices[colorAttachmentIndex];
                    if (textureIsWritten[textureIndex]) throw std::runtime_error("Cannot write to texture more than once.");
                    textureIsWritten[textureIndex] = true;
                    colorAttachmentReferences[renderPassIndex][subPassIndex].push_back({textureIndexToAttachmentIndex[textureIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
                    dependencies[renderPassIndex].push_back(VkSubpassDependency{
                        VK_SUBPASS_EXTERNAL, static_cast<unsigned int>(subPassIndex),
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,// what stages of the src must wait before giving it to the dst
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,// what stages of the dst must wait for it to be ready from the src
                        0,// what kind of acessing the src can do
                        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,// what kind of acessing the dst can do
                        VK_DEPENDENCY_BY_REGION_BIT
                    });
                    lastUsedByThisRenderPass[textureIndex] = subPassIndex;
                    lastUsedByAsThisRenderPass[textureIndex] = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                }

                bool hasDepthAttachment = (subPass.depthAttachmentIndex != SubPassConfig::NO_DEPTH_ATTACHMENT);
                if (hasDepthAttachment) {
                    std::cout << "            depth attachment\n";
                    unsigned int& textureIndex = subPass.depthAttachmentIndex;
                    textureIsWritten[textureIndex] = true;
                    depthAttachmentReferences[renderPassIndex][subPassIndex] = {textureIndexToAttachmentIndex[textureIndex], VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
                    if (lastUsedByAsThisRenderPass[textureIndex] == 0u)
                        dependencies[renderPassIndex].push_back(VkSubpassDependency{
                            VK_SUBPASS_EXTERNAL, static_cast<unsigned int>(subPassIndex),
                            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,// what stages of the src must wait before giving it to the dst
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,// what stages of the dst must wait for it to be ready from the src
                            0,// what kind of acessing the src can do
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,// what kind of acessing the dst can do
                            VK_DEPENDENCY_BY_REGION_BIT
                        });
                    else if (lastUsedByAsThisRenderPass[textureIndex] == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
                        dependencies[renderPassIndex].push_back(VkSubpassDependency{
                            lastUsedByThisRenderPass[textureIndex], static_cast<unsigned int>(subPassIndex),
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,// what stages of the src must wait before giving it to the dst
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,// what stages of the dst must wait for it to be ready from the src
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,// what kind of acessing the src can do
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,// what kind of acessing the dst can do
                            VK_DEPENDENCY_BY_REGION_BIT
                        });
                    else if (lastUsedByAsThisRenderPass[textureIndex] == VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
                        dependencies[renderPassIndex].push_back(VkSubpassDependency{
                            lastUsedByThisRenderPass[textureIndex], static_cast<unsigned int>(subPassIndex),
                            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,// what stages of the src must wait before giving it to the dst
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,// what stages of the dst must wait for it to be ready from the src
                            VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,// what kind of acessing the src can do
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,// what kind of acessing the dst can do
                            VK_DEPENDENCY_BY_REGION_BIT
                        });
                    else throw std::runtime_error("error!");
                    lastUsedByThisRenderPass[textureIndex] = subPassIndex;
                    lastUsedByAsThisRenderPass[textureIndex] = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                }
                std::cout << "        end sub pass #" << subPassIndex << ", pass two\n";
            }
            std::cout << "    end render pass #" << renderPassIndex << '\n';
        }
        std::cout << "end render passes\n";
        // check that image #0 is correct to be the swapchain output image.
        if (!textureIsWritten[0]) throw std::runtime_error("Must write to swachain output image.");
        if (textureConfigs[0].format != swapChainImageFormat) throw std::runtime_error("Swapchain output image must have default format.");
        if (textureConfigs[0].aspect != VK_IMAGE_ASPECT_COLOR_BIT) throw std::runtime_error("Swapchain output image must have default format.");
        // if (textureConfigs[0].imageLayout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) throw std::runtime_error("Swapchain output image must have color type layout.");
        // if (textureConfigs[0].attachmentLayout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) throw std::runtime_error("Swapchain output image must have color type layout.");
        if (textureIsRead[0]) throw std::runtime_error("Cannot read swapchain output image as input");
        if (textureConfigs[0].createSampler) throw std::runtime_error("Cannot read swapchain output image as input");
        // remove swapchain output image from configs
        textureConfigs.erase(textureConfigs.begin());
        return textureConfigs;
    }
    void Swapchain::createTextures(const std::vector<Swapchain::TextureConfig>& textureConfigs) {
        std::cout << "creating textures\n";
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
                textures[i][j] = new Texture(
                    device, swapChainExtent.width, swapChainExtent.height, nullptr, textureConfigs[i].format, VK_IMAGE_TILING_OPTIMAL,
                    textureConfigs[i].usage, textureConfigs[i].aspect, textureConfigs[i].imageLayout, textureConfigs[i].createSampler, VK_TRUE
                );
                textureDescriptors[i][j] = textures[i][j]->descriptorInfo();
            }
        }
        std::cout << "end creating textures\n";
    }
    void Swapchain::createRenderPass(const unsigned int& renderPassIndex) {
        std::cout << "creating render pass #" << renderPassIndex << "\n";
        std::vector<VkSubpassDescription> subpasses{};
        std::vector<SubPassConfig>& subpassConfigs = passConfigs[renderPassIndex];
        for (size_t subPassIndex = 0; subPassIndex < subpassConfigs.size(); subPassIndex++) {
            // get pointer to input attachment references
            VkAttachmentReference* pInputAttachments = nullptr;
            unsigned int inputAttachmentCount = static_cast<unsigned int>(subpassConfigs[subPassIndex].inputAttachmentIndices.size());
            if (inputAttachmentCount > 0)
                pInputAttachments = inputAttachmentReferences[renderPassIndex][subPassIndex].data();
            // setup output color attachments
            unsigned int colorAttachmentCount = static_cast<unsigned int>(subpassConfigs[subPassIndex].colorAttachmentIndices.size());
            VkAttachmentReference* pColorAttachments = nullptr;
            if (colorAttachmentCount > 0)
                pColorAttachments = colorAttachmentReferences[renderPassIndex][subPassIndex].data();
            // setup depth texture attachment(if any)
            VkAttachmentReference* pDepthAttachment = nullptr;
            if (subpassConfigs[subPassIndex].depthAttachmentIndex != SubPassConfig::NO_DEPTH_ATTACHMENT)
                pDepthAttachment = &depthAttachmentReferences[renderPassIndex][subPassIndex];
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
        renderPassInfo.attachmentCount = static_cast<unsigned int>(attachments[renderPassIndex].size());
        renderPassInfo.pAttachments = attachments[renderPassIndex].data();
        renderPassInfo.subpassCount = static_cast<unsigned int>(subpasses.size());
        renderPassInfo.pSubpasses = subpasses.data();
        renderPassInfo.dependencyCount = static_cast<unsigned int>(dependencies[renderPassIndex].size());
        renderPassInfo.pDependencies = dependencies[renderPassIndex].data();
        if (vkCreateRenderPass(device->device, &renderPassInfo, nullptr, &renderPasses[renderPassIndex]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create render pass!");
        std::cout << "end creating render pass #" << renderPassIndex << "\n";
    }
    void Swapchain::createRenderPassFrameBuffers(const unsigned int& renderPassIndex) {
        std::cout << "creating render pass #" << renderPassIndex << " frame buffer\n";
        swapChainFramebuffers[renderPassIndex].resize(imageCount);
        for (size_t i = 0; i < imageCount; i++) {
            std::vector<VkImageView> attachmentViews{};
            for (size_t& textureIndex : attachmentIndexToTextureIndex[renderPassIndex]) {
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
        std::cout << "end creating render pass #" << renderPassIndex << " frame buffer\n";
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