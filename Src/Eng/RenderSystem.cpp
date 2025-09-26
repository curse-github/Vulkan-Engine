#include "RenderSystem.h"

namespace Eng {
    RendererAbstract::RendererAbstract(Device* _device) : device(_device), pipeline(nullptr) {
        construct();
        pipelineConfig.setDefaults();
    };
    void RendererAbstract::construct() {
        if (pipeline != nullptr) {
            delete pipeline;
            vkDestroyPipelineLayout(device->device, pipelineLayout, nullptr);
        }
        descriptorSetLayouts.clear();
    }
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
        if (pipeline != nullptr) {
            delete pipeline;
            vkDestroyPipelineLayout(device->device, pipelineLayout, nullptr);
        }
    };




    RenderSystem::RenderSystem(Window* _window, Device* _device, std::vector<std::vector<SubPass>> _passes, DescriptorPool* _globalDescriptorPool)
        : window(_window), device(_device), swapchain(nullptr), passes((std::vector<std::vector<SubPass>>)_passes), globalDescriptorPool(_globalDescriptorPool)
    {
        processConfig();
        recreateSwapchain();
        createCommandBuffers();
        allocateInputAttachments();
    }

    RenderSystem::~RenderSystem() {
        delete swapchain;
    }
    
    std::vector<Texture::Config> textureConfigs{};
    std::vector<Swapchain::RenderPassConfig> renderPassConfigs{};
    std::vector<std::vector<VkClearValue>> clearValues{};
// #define CONFIG_DEBUG
    void RenderSystem::processConfig() {
#if defined(CONFIG_DEBUG)
        std::cout << "process config\n";
#endif
        size_t texturesNeeded = 2;
        textureConfigs.clear();
        textureConfigs.resize(texturesNeeded);
        std::vector<bool> textureIsWritten(texturesNeeded);
        std::vector<bool> textureIsRead(texturesNeeded);
        
        size_t numRenderPasses = passes.size();
        renderPassConfigs.clear();
        renderPassConfigs.resize(numRenderPasses);
        clearValues.clear();
        clearValues.resize(numRenderPasses);
        inputAttachmentDescriptorSetLayouts.clear();
        inputAttachmentDescriptorSetLayouts.resize(numRenderPasses);
        sampledInputDescriptorSetLayouts.clear();
        sampledInputDescriptorSetLayouts.resize(numRenderPasses);
        inputAttachmentDescriptorSets.clear();
        inputAttachmentDescriptorSets.resize(numRenderPasses);
        sampledInputDescriptorSets.clear();
        sampledInputDescriptorSets.resize(numRenderPasses);
#if defined(CONFIG_DEBUG)
        std::cout << "render passes\n";
#endif
        for (size_t renderPassIndex = 0; renderPassIndex < numRenderPasses; renderPassIndex++) {
#if defined(CONFIG_DEBUG)
            std::cout << "    render pass #" << renderPassIndex << ", read/write/(texture format) check pass\n";
#endif
            std::vector<SubPass>& subpassConfigs = passes[renderPassIndex];
            size_t numSubPasses = subpassConfigs.size();
            renderPassConfigs[renderPassIndex].subPassConfigs.resize(numSubPasses);
            inputAttachmentDescriptorSetLayouts[renderPassIndex].resize(numSubPasses);
            sampledInputDescriptorSetLayouts[renderPassIndex].resize(numSubPasses);
            inputAttachmentDescriptorSets[renderPassIndex].resize(numSubPasses);
            sampledInputDescriptorSets[renderPassIndex].resize(numSubPasses);
            for (size_t subPassIndex = 0; subPassIndex < numSubPasses; subPassIndex++) {
#if defined(CONFIG_DEBUG)
                std::cout << "        sub pass #" << subPassIndex << "\n";
#endif
                SubPass& subPass = subpassConfigs[subPassIndex];
                std::vector<unsigned int>& sampledInputIndices = subPass.sampledInputIndices;
                size_t numSampledInputs = sampledInputIndices.size();
                if (numSampledInputs > 0) {
                    DescriptorSetLayout::Builder layoutBuilder(device);
                    for (size_t sampledInputIndex = 0; sampledInputIndex < numSampledInputs; sampledInputIndex++) {
#if defined(CONFIG_DEBUG)
                        std::cout << "            sampled input #" << sampledInputIndex << '\n';
#endif
                        unsigned int& textureIndex = sampledInputIndices[sampledInputIndex];
                        if (textureIndex >= texturesNeeded) {
                            texturesNeeded = textureIndex+1;
                            textureConfigs.resize(texturesNeeded);
                            textureIsWritten.resize(texturesNeeded, false);
                            textureIsRead.resize(texturesNeeded, false);
                        }
                        if (!textureIsWritten[textureIndex]) throw std::runtime_error("Cannot read a texture before it has been written to.");
                        textureIsRead[textureIndex] = true;
                        textureConfigs[textureIndex].usage = textureConfigs[textureIndex].usage|VK_IMAGE_USAGE_SAMPLED_BIT;
                        textureConfigs[textureIndex].createSampler = true;
                        textureConfigs[textureIndex].unnormalizedCoordinates = true;
                        textureConfigs[textureIndex].layout = (((textureConfigs[textureIndex].aspect == VK_IMAGE_ASPECT_DEPTH_BIT)) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                        layoutBuilder.addBinding(sampledInputIndex, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
                    }
                    sampledInputDescriptorSetLayouts[renderPassIndex][subPassIndex] = layoutBuilder.build();
                }

                std::vector<unsigned int>& inputAttachmentIndices = subPass.inputAttachmentIndices;
                size_t numInputAttachments = inputAttachmentIndices.size();
                if (numInputAttachments > 0) {
                    DescriptorSetLayout::Builder layoutBuilder(device);
                    for (size_t inputAttachmentIndex = 0; inputAttachmentIndex < numInputAttachments; inputAttachmentIndex++) {
#if defined(CONFIG_DEBUG)
                        std::cout << "            input attachment #" << inputAttachmentIndex << '\n';
#endif
                        unsigned int& textureIndex = inputAttachmentIndices[inputAttachmentIndex];
                        if (textureIndex >= texturesNeeded) {
                            texturesNeeded = textureIndex+1;
                            textureConfigs.resize(texturesNeeded);
                            textureIsWritten.resize(texturesNeeded, false);
                            textureIsRead.resize(texturesNeeded, false);
                        }
                        if (!textureIsWritten[textureIndex]) throw std::runtime_error("Cannot read a texture before it has been written to.");
                        textureIsRead[textureIndex] = true;
                        textureConfigs[textureIndex].usage = textureConfigs[textureIndex].usage|VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
                        textureConfigs[textureIndex].layout = (((textureConfigs[textureIndex].aspect == VK_IMAGE_ASPECT_DEPTH_BIT)) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                        layoutBuilder.addBinding(inputAttachmentIndex, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
                    }
                    inputAttachmentDescriptorSetLayouts[renderPassIndex][subPassIndex] = layoutBuilder.build();
                }

                std::vector<unsigned int>& colorAttachmentIndices = subPass.colorAttachmentIndices;
                size_t numColorAttachments = colorAttachmentIndices.size();
                for (size_t colorAttachmentIndex = 0; colorAttachmentIndex < numColorAttachments; colorAttachmentIndex++) {
#if defined(CONFIG_DEBUG)
                    std::cout << "            color attachment #" << colorAttachmentIndex << '\n';
#endif
                    unsigned int& textureIndex = colorAttachmentIndices[colorAttachmentIndex];
                    if (textureIndex >= texturesNeeded) {
                        texturesNeeded = textureIndex+1;
                        textureConfigs.resize(texturesNeeded);
                        textureIsWritten.resize(texturesNeeded, false);
                        textureIsRead.resize(texturesNeeded, false);
                    }
                    textureConfigs[textureIndex] = Texture::Config::createColorImage();
                    if (textureIsWritten[textureIndex]) throw std::runtime_error("Cannot write to color texture more than once.");
                    textureIsWritten[textureIndex] = true;
                }

                bool hasDepthAttachment = (subPass.depthAttachmentIndex != Swapchain::SubPassConfig::NO_DEPTH_ATTACHMENT);
                if (hasDepthAttachment) {
#if defined(CONFIG_DEBUG)
                    std::cout << "            depth attachment\n";
#endif
                    unsigned int& textureIndex = subPass.depthAttachmentIndex;
                    if (textureIndex >= texturesNeeded) {
                        texturesNeeded = textureIndex+1;
                        textureConfigs.resize(texturesNeeded);
                        textureIsWritten.resize(texturesNeeded, false);
                        textureIsRead.resize(texturesNeeded, false);
                    }
                    textureConfigs[textureIndex] = Texture::Config::createDepthTexture();
                    textureIsWritten[textureIndex] = true;
                }
#if defined(CONFIG_DEBUG)
                std::cout << "        end sub pass #" << subPassIndex << "\n";
#endif
            }
#if defined(CONFIG_DEBUG)
            std::cout << "    end render pass #" << renderPassIndex << ", read/write/(texture format) check pass\n";
#endif
        }
#if defined(CONFIG_DEBUG)
        std::cout << "render passes again\n";
#endif
        for (size_t renderPassIndex = 0; renderPassIndex < numRenderPasses; renderPassIndex++) {
#if defined(CONFIG_DEBUG)
            std::cout << "    render pass #" << renderPassIndex << ", (clear value)/attachment/dependency generator\n";
#endif
            std::vector<SubPass>& subpassConfigs = passes[renderPassIndex];
            size_t numSubPasses = subpassConfigs.size();
            std::vector<unsigned int> lastUsedByThisRenderPass(texturesNeeded, VK_SUBPASS_EXTERNAL);
            for (size_t subPassIndex = 0; subPassIndex < numSubPasses; subPassIndex++) {
#if defined(CONFIG_DEBUG)
                std::cout << "        sub pass #" << subPassIndex << ", usage check pass\n";
#endif
                SubPass& subPass = subpassConfigs[subPassIndex];
                std::vector<unsigned int>& sampledInputIndices = subPass.sampledInputIndices;
                size_t numSampledInputs = sampledInputIndices.size();
                for (size_t sampledInputIndex = 0; sampledInputIndex < numSampledInputs; sampledInputIndex++) {
#if defined(CONFIG_DEBUG)
                    std::cout << "            sampled input #" << sampledInputIndex << '\n';
#endif
                    unsigned int& textureIndex = sampledInputIndices[sampledInputIndex];
                    if (lastUsedByThisRenderPass[textureIndex] == subPassIndex) throw std::runtime_error("Cannot use texture more than once in a subpass.");
                }

                std::vector<unsigned int>& inputAttachmentIndices = subPass.inputAttachmentIndices;
                size_t numInputAttachments = inputAttachmentIndices.size();
                for (size_t inputAttachmentIndex = 0; inputAttachmentIndex < numInputAttachments; inputAttachmentIndex++) {
#if defined(CONFIG_DEBUG)
                    std::cout << "            input attachment #" << inputAttachmentIndex << '\n';
#endif
                    unsigned int& textureIndex = inputAttachmentIndices[inputAttachmentIndex];
                    if (lastUsedByThisRenderPass[textureIndex] == subPassIndex) throw std::runtime_error("Cannot use texture more than once in a subpass.");
                    lastUsedByThisRenderPass[textureIndex] = subPassIndex;
                }

                std::vector<unsigned int>& colorAttachmentIndices = subPass.colorAttachmentIndices;
                size_t numColorAttachments = colorAttachmentIndices.size();
                for (size_t colorAttachmentIndex = 0; colorAttachmentIndex < numColorAttachments; colorAttachmentIndex++) {
#if defined(CONFIG_DEBUG)
                    std::cout << "            color attachment #" << colorAttachmentIndex << '\n';
#endif
                    unsigned int& textureIndex = colorAttachmentIndices[colorAttachmentIndex];
                    if (lastUsedByThisRenderPass[textureIndex] == subPassIndex) throw std::runtime_error("Cannot use texture more than once in a subpass.");
                    lastUsedByThisRenderPass[textureIndex] = subPassIndex;
                }

                bool hasDepthAttachment = (subPass.depthAttachmentIndex != Swapchain::SubPassConfig::NO_DEPTH_ATTACHMENT);
                if (hasDepthAttachment) {
#if defined(CONFIG_DEBUG)
                    std::cout << "            depth attachment\n";
#endif
                    unsigned int& textureIndex = subPass.depthAttachmentIndex;
                    if (lastUsedByThisRenderPass[textureIndex] == subPassIndex) throw std::runtime_error("Cannot use texture more than once in a subpass.");
                    lastUsedByThisRenderPass[textureIndex] = subPassIndex;
                }
#if defined(CONFIG_DEBUG)
                std::cout << "        end sub pass #" << subPassIndex << ", usage check pass\n";
#endif
            }
            std::vector<unsigned int> textureIndexToAttachmentIndex(texturesNeeded);
#if defined(CONFIG_DEBUG)
            std::cout << "        texture (clear value)/attachment generation\n";
#endif
            for (size_t textureIndex = 0; textureIndex < lastUsedByThisRenderPass.size(); textureIndex++) {
#if defined(CONFIG_DEBUG)
                std::cout << "            texture #" << textureIndex << '\n';
#endif
                if (lastUsedByThisRenderPass[textureIndex] == VK_SUBPASS_EXTERNAL) continue;
#if defined(CONFIG_DEBUG)
                std::cout << "                clear value\n";
#endif
                if (textureConfigs[textureIndex].aspect == VK_IMAGE_ASPECT_DEPTH_BIT)
                    clearValues[renderPassIndex].push_back(VkClearValue{ depthStencil:{1.0f, 0u} });
                else
                    clearValues[renderPassIndex].push_back(VkClearValue{ color:{0.0f, 0.0f, 0.0f, 1.0f} });
#if defined(CONFIG_DEBUG)
                std::cout << "                attachment\n";
#endif
                renderPassConfigs[renderPassIndex].attachmentIndexToTextureIndex.push_back(static_cast<unsigned int>(textureIndex));
                textureIndexToAttachmentIndex[textureIndex] = static_cast<unsigned int>(renderPassConfigs[renderPassIndex].attachments.size());
                if(textureIndex == 0)
                    renderPassConfigs[renderPassIndex].attachments.push_back(VkAttachmentDescription{
                        0,// flags
                        VK_FORMAT_UNDEFINED,
                        VK_SAMPLE_COUNT_1_BIT,
                        VK_ATTACHMENT_LOAD_OP_CLEAR,
                        VK_ATTACHMENT_STORE_OP_STORE,
                        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                        VK_ATTACHMENT_STORE_OP_DONT_CARE,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
                    });
                else {
                    //VkLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    //if (textureIsRead[textureIndex])
                    //    initialLayout = (((textureConfigs[textureIndex].aspect == VK_IMAGE_ASPECT_DEPTH_BIT)) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                    renderPassConfigs[renderPassIndex].attachments.push_back(VkAttachmentDescription{
                        0,// flags
                        VK_FORMAT_UNDEFINED,
                        VK_SAMPLE_COUNT_1_BIT,
                        VK_ATTACHMENT_LOAD_OP_CLEAR,// ((textureConfigs[textureIndex].aspect == VK_IMAGE_ASPECT_DEPTH_BIT) ? VK_ATTACHMENT_LOAD_OP_DONT_CARE : VK_ATTACHMENT_LOAD_OP_CLEAR),
                        VK_ATTACHMENT_STORE_OP_STORE,
                        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                        VK_ATTACHMENT_STORE_OP_DONT_CARE,
                        textureConfigs[textureIndex].layout,
                        textureConfigs[textureIndex].layout
                    });
                }
            }
#if defined(CONFIG_DEBUG)
            std::cout << "        end texture (clear value)/attachment generation\n";
#endif
            renderPassConfigs[renderPassIndex].subPassConfigs.resize(numSubPasses);
            for (size_t i = 0; i < lastUsedByThisRenderPass.size(); i++) lastUsedByThisRenderPass[i] = VK_SUBPASS_EXTERNAL;
            std::vector<VkImageUsageFlags> lastUsedByAsThisRenderPass(texturesNeeded, 0u);
            for (size_t subPassIndex = 0; subPassIndex < numSubPasses; subPassIndex++) {
#if defined(CONFIG_DEBUG)
                std::cout << "        sub pass #" << subPassIndex << ", dependency generation pass\n";
#endif
                SubPass& subPass = subpassConfigs[subPassIndex];
                std::vector<unsigned int>& sampledInputIndices = subPass.sampledInputIndices;
                size_t numSampledInputs = sampledInputIndices.size();
                for (size_t sampledInputIndex = 0; sampledInputIndex < numSampledInputs; sampledInputIndex++) {
#if defined(CONFIG_DEBUG)
                    std::cout << "            sampled input #" << sampledInputIndex << '\n';
#endif
                    unsigned int& textureIndex = sampledInputIndices[sampledInputIndex];
                    if (lastUsedByThisRenderPass[textureIndex] != VK_SUBPASS_EXTERNAL) throw std::runtime_error("Cannot sample texture written in the same render pass.");
                }

                std::vector<unsigned int>& inputAttachmentIndices = subPass.inputAttachmentIndices;
                size_t numInputAttachments = inputAttachmentIndices.size();
                for (size_t inputAttachmentIndex = 0; inputAttachmentIndex < numInputAttachments; inputAttachmentIndex++) {
#if defined(CONFIG_DEBUG)
                    std::cout << "            input attachment #" << inputAttachmentIndex << '\n';
#endif
                    unsigned int& textureIndex = inputAttachmentIndices[inputAttachmentIndex];
                    VkImageLayout layout = ((textureConfigs[textureIndex].aspect == VK_IMAGE_ASPECT_DEPTH_BIT) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                    renderPassConfigs[renderPassIndex].subPassConfigs[subPassIndex].inputAttachmentReferences.push_back({textureIndexToAttachmentIndex[textureIndex], layout});
                    if (lastUsedByThisRenderPass[textureIndex] != VK_SUBPASS_EXTERNAL) {
                        if (lastUsedByAsThisRenderPass[textureIndex] == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
                            renderPassConfigs[renderPassIndex].dependencies.push_back(VkSubpassDependency{
                                lastUsedByThisRenderPass[textureIndex], static_cast<unsigned int>(subPassIndex),
                                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,// what stages of the src must wait before giving it to the dst
                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,// what stages of the dst must wait for it to be ready from the src
                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,// what kind of acessing the src can do
                                VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,// what kind of acessing the dst can do
                                VK_DEPENDENCY_BY_REGION_BIT
                            });
                        else if (lastUsedByAsThisRenderPass[textureIndex] == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
                            renderPassConfigs[renderPassIndex].dependencies.push_back(VkSubpassDependency{
                                lastUsedByThisRenderPass[textureIndex], static_cast<unsigned int>(subPassIndex),
                                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,// what stages of the src must wait before giving it to the dst
                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,// what stages of the dst must wait for it to be ready from the src
                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,// what kind of acessing the src can do
                                VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,// what kind of acessing the dst can do
                                VK_DEPENDENCY_BY_REGION_BIT
                            });
                        else if (lastUsedByAsThisRenderPass[textureIndex] == VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
                            renderPassConfigs[renderPassIndex].dependencies.push_back(VkSubpassDependency{
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
#if defined(CONFIG_DEBUG)
                    std::cout << "            color attachment #" << colorAttachmentIndex << '\n';
#endif
                    unsigned int& textureIndex = colorAttachmentIndices[colorAttachmentIndex];
                    renderPassConfigs[renderPassIndex].subPassConfigs[subPassIndex].colorAttachmentReferences.push_back({textureIndexToAttachmentIndex[textureIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
                    renderPassConfigs[renderPassIndex].dependencies.push_back(VkSubpassDependency{
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

                bool hasDepthAttachment = (subPass.depthAttachmentIndex != Swapchain::SubPassConfig::NO_DEPTH_ATTACHMENT);
                if (hasDepthAttachment) {
#if defined(CONFIG_DEBUG)
                    std::cout << "            depth attachment\n";
#endif
                    unsigned int& textureIndex = subPass.depthAttachmentIndex;
                    renderPassConfigs[renderPassIndex].subPassConfigs[subPassIndex].hasDepthAttachment = true;
                    renderPassConfigs[renderPassIndex].subPassConfigs[subPassIndex].depthAttachmentReferences = {textureIndexToAttachmentIndex[textureIndex], VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
                    if (lastUsedByAsThisRenderPass[textureIndex] == 0u)
                        renderPassConfigs[renderPassIndex].dependencies.push_back(VkSubpassDependency{
                            VK_SUBPASS_EXTERNAL, static_cast<unsigned int>(subPassIndex),
                            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,// what stages of the src must wait before giving it to the dst
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,// what stages of the dst must wait for it to be ready from the src
                            0,// what kind of acessing the src can do
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,// what kind of acessing the dst can do
                            VK_DEPENDENCY_BY_REGION_BIT
                        });
                    else if (lastUsedByAsThisRenderPass[textureIndex] == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
                        renderPassConfigs[renderPassIndex].dependencies.push_back(VkSubpassDependency{
                            lastUsedByThisRenderPass[textureIndex], static_cast<unsigned int>(subPassIndex),
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,// what stages of the src must wait before giving it to the dst
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,// what stages of the dst must wait for it to be ready from the src
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,// what kind of acessing the src can do
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,// what kind of acessing the dst can do
                            VK_DEPENDENCY_BY_REGION_BIT
                        });
                    else if (lastUsedByAsThisRenderPass[textureIndex] == VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
                        renderPassConfigs[renderPassIndex].dependencies.push_back(VkSubpassDependency{
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
#if defined(CONFIG_DEBUG)
                std::cout << "        end sub pass #" << subPassIndex << ", dependency generation pass\n";
#endif
            }
#if defined(CONFIG_DEBUG)
            std::cout << "    end render pass #" << renderPassIndex << '\n';
#endif
        }
        // check that image #0 is correct to be the swapchain output image.
        if (!textureIsWritten[0]) throw std::runtime_error("Must write to swachain output image.");
        if (textureConfigs[0].aspect != VK_IMAGE_ASPECT_COLOR_BIT) throw std::runtime_error("Swapchain output image must have default format.");
        if ((textureConfigs[0].layout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) && (textureConfigs[0].layout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)) throw std::runtime_error("Swapchain output image must have color type layout.");
        if (textureIsRead[0]) throw std::runtime_error("Cannot read swapchain output image as input");
        if (textureConfigs[0].createSampler) throw std::runtime_error("Cannot read swapchain output image as input");
        // remove swapchain output image from configs
        textureConfigs.erase(textureConfigs.begin());
    }
    void RenderSystem::recreateSwapchain(const bool& attemptRecreate) {
        // IMPORTANT: this halts the program while minimized.
        while ((window->size.x == 0) || (window->size.y == 0))
            glfwWaitEvents();
        vkDeviceWaitIdle(device->device);
        // recreate the swapchain
        if ((swapchain == nullptr)||(!attemptRecreate)) swapchain = new Swapchain(device, VkExtent2D{static_cast<unsigned int>(window->size.x), static_cast<unsigned int>(window->size.y)}, textureConfigs, renderPassConfigs);
        else {
            std::cout << "Recreating swapchain\n";
            Swapchain* oldSwapchain = swapchain;
            swapchain = new Swapchain(device, VkExtent2D{static_cast<unsigned int>(window->size.x), static_cast<unsigned int>(window->size.y)}, textureConfigs, renderPassConfigs, oldSwapchain);
            delete oldSwapchain;
            if (!oldSwapchain->swapchainsCompatible(*swapchain))
                // should at some point just recreate the pipeline/rendersystems
                throw std::runtime_error("Swapchain image format has changed!");
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
        renderPassInfo.clearValueCount = static_cast<unsigned int>(clearValues[_renderPassIndex].size());
        renderPassInfo.pClearValues = clearValues[_renderPassIndex].data();
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
        size_t numRenderPasses = passes.size();
        for (size_t i = 0; i < numRenderPasses; i++) {
            VkRenderPass renderPass = swapchain->renderPasses[i];
            size_t numSubPasses = passes[i].size();
            for (size_t j = 0; j < numSubPasses; j++) {
                size_t numInputsAttachments = passes[i][j].inputAttachmentIndices.size();
                size_t numSampledInputs = passes[i][j].sampledInputIndices.size();
                size_t numRenderers = passes[i][j].renderers.size();
                for (size_t k = 0; k < numRenderers; k++) {
                    if (numInputsAttachments > 0) passes[i][j].renderers[k]->descriptorSetLayouts.push_back(inputAttachmentDescriptorSetLayouts[i][j]->descriptorSetLayout);
                    if (numSampledInputs > 0) passes[i][j].renderers[k]->descriptorSetLayouts.push_back(sampledInputDescriptorSetLayouts[i][j]->descriptorSetLayout);
                    passes[i][j].renderers[k]->init(renderPass, j);
                }
            }
        }
        for (size_t i = 0; i < numRenderPasses; i++) {
            size_t numSubPasses = passes[i].size();
            for (size_t j = 0; j < numSubPasses; j++) {
                size_t numInputsAttachments = passes[i][j].inputAttachmentIndices.size();
                if (numInputsAttachments > 0) {
                    size_t imageCount = swapchain->getImageCount();
                    inputAttachmentDescriptorSets[i][j].resize(imageCount, VK_NULL_HANDLE);
                    for (size_t l = 0; l < swapchain->getImageCount(); l++) {
                        DescriptorWriter writer(inputAttachmentDescriptorSetLayouts[i][j], globalDescriptorPool);
                        for (size_t k = 0; k < numInputsAttachments; k++)
                            writer.writeImage(k, &swapchain->textureDescriptors[passes[i][j].inputAttachmentIndices[k]-1u][i]);
                        writer.build(inputAttachmentDescriptorSets[i][j][l]);
                    }
                }
                size_t numSampledInputs = passes[i][j].sampledInputIndices.size();
                if (numSampledInputs > 0) {
                    size_t imageCount = swapchain->getImageCount();
                    sampledInputDescriptorSets[i][j].resize(imageCount, VK_NULL_HANDLE);
                    for (size_t l = 0; l < swapchain->getImageCount(); l++) {
                        DescriptorWriter writer(sampledInputDescriptorSetLayouts[i][j], globalDescriptorPool);
                        for (size_t k = 0; k < numSampledInputs; k++)
                            writer.writeImage(k, &swapchain->textureDescriptors[passes[i][j].sampledInputIndices[k]-1u][i]);
                        writer.build(sampledInputDescriptorSets[i][j][l]);
                    }
                }
            }
        }
    }
    void RenderSystem::overwriteInputAttachments() {
        size_t numRenderPasses = passes.size();
        for (size_t i = 0; i < numRenderPasses; i++) {
            size_t numSubPasses = passes[i].size();
            for (size_t j = 0; j < numSubPasses; j++) {
                size_t numInputsAttachments = passes[i][j].inputAttachmentIndices.size();
                if (numInputsAttachments > 0) {
                    size_t imageCount = swapchain->getImageCount();
                    inputAttachmentDescriptorSets[i][j].resize(imageCount, VK_NULL_HANDLE);
                    for (size_t l = 0; l < swapchain->getImageCount(); l++) {
                        DescriptorWriter writer(inputAttachmentDescriptorSetLayouts[i][j], globalDescriptorPool);
                        for (size_t k = 0; k < numInputsAttachments; k++)
                            writer.writeImage(k, &swapchain->textureDescriptors[passes[i][j].inputAttachmentIndices[k]-1u][i]);
                        writer.overwrite(inputAttachmentDescriptorSets[i][j][l]);
                    }
                }
                size_t numSampledInputs = passes[i][j].sampledInputIndices.size();
                if (numSampledInputs > 0) {
                    size_t imageCount = swapchain->getImageCount();
                    sampledInputDescriptorSets[i][j].resize(imageCount, VK_NULL_HANDLE);
                    for (size_t l = 0; l < swapchain->getImageCount(); l++) {
                        DescriptorWriter writer(sampledInputDescriptorSetLayouts[i][j], globalDescriptorPool);
                        for (size_t k = 0; k < numSampledInputs; k++)
                            writer.writeImage(k, &swapchain->textureDescriptors[passes[i][j].sampledInputIndices[k]-1u][i]);
                        writer.overwrite(sampledInputDescriptorSets[i][j][l]);
                    }
                }
            }
        }
    }


    void RenderSystem::setConfig(std::vector<std::vector<SubPass>> _passes) {
        vkDeviceWaitIdle(device->device);
        passes = _passes;
        processConfig();
        // recreate the swapchain
        std::cout << "Recreating swapchain\n";
        delete swapchain;
        swapchain = new Swapchain(device, VkExtent2D{static_cast<unsigned int>(window->size.x), static_cast<unsigned int>(window->size.y)}, textureConfigs, renderPassConfigs);
        for (size_t i = 0; i < passes.size(); i++) {
            size_t numSubPasses = passes[i].size();
            for (size_t j = 0; j < numSubPasses; j++) {
                for (size_t k = 0; k < passes[i][j].renderers.size(); k++) {
                    passes[i][j].renderers[k]->construct();
                }
            }
        }
        allocateInputAttachments();
        scissor.extent = swapchain->swapChainExtent;
        viewport.width = static_cast<float>(scissor.extent.width);
        viewport.height = static_cast<float>(scissor.extent.height);
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
        for (size_t i = 0; i < passes.size(); i++) {
            size_t numSubPasses = passes[i].size();
            for (size_t j = 0; j < numSubPasses; j++) {
                std::vector<VkDescriptorSet> descriptorSets{};
                std::vector<unsigned int>& sampledInputIndices = passes[i][j].sampledInputIndices;
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
                if (passes[i][j].sampledInputIndices.size() > 0) descriptorSets.push_back(sampledInputDescriptorSets[i][j][imageIndex]);
                if (passes[i][j].inputAttachmentIndices.size() > 0) descriptorSets.push_back(inputAttachmentDescriptorSets[i][j][imageIndex]);
                for (size_t k = 0; k < passes[i][j].renderers.size(); k++) {
                    if (descriptorSets.size() > 0)
                        vkCmdBindDescriptorSets(
                            frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, passes[i][j].renderers[k]->pipelineLayout,
                            1, static_cast<unsigned int>(descriptorSets.size()), descriptorSets.data(), 0, nullptr
                        );
                    passes[i][j].renderers[k]->render(frameInfo);
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