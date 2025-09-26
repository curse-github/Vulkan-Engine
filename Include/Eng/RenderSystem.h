#ifndef ENG_RENDERER
#define ENG_RENDERER

#include "Helpers.h"
#include "Window.h"
#include "Device.h"
#include "Swapchain.h"
#include "Mesh.h"
#include "Descriptors.h"
#include "Pipeline.h"
#include "FrameInfo.h"

namespace Eng {
    class RenderSystem;
    class RendererAbstract {
    protected:
        Device* device;
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        PipelineConfigInfo pipelineConfig{};
        VkPipelineLayout pipelineLayout;
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{};
        std::vector<VkPushConstantRange> pushConstantRanges{};
        std::string vertShaderFile;
        std::string fragShaderFile;
        Pipeline* pipeline;
        friend RenderSystem;
    public:
        RendererAbstract(Device* _device);
        virtual void construct();
        void init(VkRenderPass& renderPass, const unsigned int& subPassIndex);
        virtual ~RendererAbstract();
        virtual void render(FrameInfo& frameInfo) = 0;
    };
    struct DefaultPushConstantData {
        glm::mat4 modelMat{1.0f};
        glm::mat4 normalMat{1.0f};
    };

    class RenderSystem {
    public:
        struct SubPass{
            static inline constexpr unsigned int NO_DEPTH_ATTACHMENT = ~0u;
            std::vector<unsigned int> sampledInputIndices{};
            std::vector<unsigned int> inputAttachmentIndices{};
            std::vector<unsigned int> colorAttachmentIndices{};
            unsigned int depthAttachmentIndex = NO_DEPTH_ATTACHMENT;
            std::vector<RendererAbstract*> renderers;

            SubPass(
                const std::vector<unsigned int>& _sampledInputIndices, const std::vector<unsigned int>& _inputAttachmentIndices, const std::vector<unsigned int>& _colorAttachmentIndices,
                const unsigned int& _depthAttachmentIndex, const std::vector<RendererAbstract*>& _renderers
            ) : sampledInputIndices(_sampledInputIndices), inputAttachmentIndices(_inputAttachmentIndices), colorAttachmentIndices(_colorAttachmentIndices),
            depthAttachmentIndex(_depthAttachmentIndex), renderers(_renderers) {};
            SubPass(const SubPass& copy) = default;
            SubPass& operator=(const SubPass& copy) = default;
            SubPass(SubPass&& move) = default;
            SubPass& operator=(SubPass&& move) = default;
            ~SubPass() = default;
        };
    private:
        Window* window;
        Device* device;
        Swapchain* swapchain;
        std::vector<VkCommandBuffer> commandBuffers;

        std::vector<std::vector<SubPass>> passes;
        DescriptorPool* globalDescriptorPool;
        std::vector<std::vector<OwnedPointer<DescriptorSetLayout>>> inputAttachmentDescriptorSetLayouts;
        std::vector<std::vector<OwnedPointer<DescriptorSetLayout>>> sampledInputDescriptorSetLayouts;
        std::vector<std::vector<std::vector<VkDescriptorSet>>> inputAttachmentDescriptorSets;
        std::vector<std::vector<std::vector<VkDescriptorSet>>> sampledInputDescriptorSets;

        void processConfig();
        void recreateSwapchain(const bool& attemptRecreate = true);
        void createCommandBuffers();
        void freeCommandBuffers();
        
        bool frameInProgress = false;
        unsigned int imageIndex;
        unsigned int renderPassIndex = ~0u;
        unsigned int frameCount = 0u;
        void beginRenderPass(VkCommandBuffer commandBuffer, const unsigned int& _renderPassIndex);
        void nextSubPass(VkCommandBuffer commandBuffer);
        void endRenderPass(VkCommandBuffer commandBuffer);
        
        VkViewport viewport{
            0.0f, 0.0f,// x and y
            1.0f, 1.0f,// width and height
            0.0f, 1.0f// min and max depth
        };
        VkRect2D scissor{
            {0u, 0u},// offset
            {1u, 1u}// extent
        };
        
        void allocateInputAttachments();
        void overwriteInputAttachments();
    public:
        RenderSystem(Window* _window, Device* _device, std::vector<std::vector<SubPass>> _passes, DescriptorPool* _globalDescriptorPool);
        RenderSystem(const RenderSystem& copy) = delete;
        RenderSystem& operator=(const RenderSystem& copy) = delete;
        RenderSystem(RenderSystem&& move) = delete;
        RenderSystem& operator=(RenderSystem&& move) = delete;
        ~RenderSystem();
        
        void setConfig(std::vector<std::vector<SubPass>> _passes);
        VkCommandBuffer beginFrame();
        void render(FrameInfo& frameInfo);
        void endFrame();
        
        unsigned int getImageCount() { return swapchain->imageCount; };
        unsigned int getImage() {
            assert(frameInProgress && "Cannot get frame when frame has not started");
            return imageIndex;
        };
        unsigned int getFrameCount() { return Swapchain::MAX_FRAMES_IN_FLIGHT; };
        unsigned int getFrame() {
            assert(frameInProgress && "Cannot get frame when frame has not started");
            return swapchain->currentFrame;
        };
        vec2 getResolution() { return {swapchain->swapChainExtent.width, swapchain->swapChainExtent.height}; };
        float getAspectRatio() { return (float)swapchain->swapChainExtent.width/swapchain->swapChainExtent.height; };
    };
}

#endif// ENG_RENDERER