#ifndef ENG_RENDERER
#define ENG_RENDERER

#include "Helpers.h"
#include "Window.h"
#include "Device.h"
#include "Swapchain.h"
#include "Mesh.h"
#include "Descriptors.h"

namespace Eng {
    class PostProcessRenderer;
    class RenderSystem {
        Window* window;
        Device* device;
        Swapchain* swapchain;
        DescriptorPool* globalDescriptorPool;
        std::vector<VkCommandBuffer> commandBuffers;
        bool frameInProgress = false;
        unsigned int imageIndex;
        unsigned int renderPassIndex = ~0u;

        void recreateSwapchain();
        void freeCommandBuffers();
        void createCommandBuffers();
    public:
        RenderSystem(Window* _window, Device* _device);
        RenderSystem(const RenderSystem& copy) = delete;
        RenderSystem& operator=(const RenderSystem& copy) = delete;
        RenderSystem(RenderSystem&& move) = delete;
        RenderSystem& operator=(RenderSystem&& move) = delete;
        ~RenderSystem();

        OwnedPointer<DescriptorSetLayout> inputAttachmentDescriptorSetLayout;
        std::vector<VkDescriptorSet> inputAttachmentDescriptorSets;
        void allocateInputAttachments(DescriptorPool* _globalDescriptorPool);
        void overwriteInputAttachments();

        VkCommandBuffer beginFrame();
        void beginRenderPass(VkCommandBuffer commandBuffer, const unsigned int& _renderPassIndex);
        void nextSubPass(VkCommandBuffer commandBuffer);
        void endRenderPass(VkCommandBuffer commandBuffer);
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
        VkCommandBuffer getCurrentCommandBuffer() {
            assert(frameInProgress && "Cannot get command buffer when frame has not started");
            return commandBuffers[swapchain->currentFrame];
        }
        VkRenderPass getRenderPass(const unsigned int& _renderPassIndex) {
            return swapchain->renderPasses[_renderPassIndex];
        }
        float getAspectRatio() { return (float)swapchain->swapChainExtent.width/swapchain->swapChainExtent.height; };

        VkViewport viewport{
            0.0f, 0.0f,// x and y
            1.0f, 1.0f,// width and height
            0.0f, 1.0f// min and max depth
        };
        VkRect2D scissor{
            {0u, 0u},// offset
            {1u, 1u}// extent
        };
    };
}

#endif// ENG_RENDERER