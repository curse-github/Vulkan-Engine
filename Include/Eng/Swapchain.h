#ifndef ENG_SWAPCHAIN
#define ENG_SWAPCHAIN

#include "Helpers.h"
#include "Device.h"
#include "Texture.h"

namespace Eng {
    class Swapchain {
    public:
        struct SubPassConfig {
        public:
            static inline constexpr unsigned int NO_DEPTH_ATTACHMENT = ~0u;
            std::vector<VkAttachmentReference> inputAttachmentReferences;
            std::vector<VkAttachmentReference> colorAttachmentReferences;
            bool hasDepthAttachment = false;
            VkAttachmentReference depthAttachmentReferences;

            SubPassConfig() = default;
            SubPassConfig(const SubPassConfig& copy) = default;
            SubPassConfig& operator=(const SubPassConfig& copy) = default;
            SubPassConfig(SubPassConfig&& move) = default;
            SubPassConfig& operator=(SubPassConfig&& move) = default;
            ~SubPassConfig() = default;
        };
        struct RenderPassConfig {
            std::vector<SubPassConfig> subPassConfigs;
            std::vector<VkAttachmentDescription> attachments;
            std::vector<size_t> attachmentIndexToTextureIndex;
            std::vector<VkSubpassDependency> dependencies;

            RenderPassConfig() = default;
            RenderPassConfig(const RenderPassConfig& copy) = default;
            RenderPassConfig& operator=(const RenderPassConfig& copy) = default;
            RenderPassConfig(RenderPassConfig&& move) = default;
            RenderPassConfig& operator=(RenderPassConfig&& move) = default;
            ~RenderPassConfig() = default;
        };

        std::vector<VkImage> swapChainImages;
        std::vector<VkImageView> swapChainImageViews;
        unsigned int currentFrame = 0;
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
        unsigned int imageCount = 0;
        std::vector<std::vector<Texture*>> textures;
        std::vector<std::vector<VkDescriptorImageInfo>> textureDescriptors;

        std::vector<std::vector<VkFramebuffer>> swapChainFramebuffers;
        std::vector<VkRenderPass> renderPasses;
        VkExtent2D swapChainExtent;

        Swapchain(Device* _device, VkExtent2D extent, const std::vector<Texture::Config>& textureConfigs, const std::vector<RenderPassConfig>& passConfigs);
        Swapchain(Device* _device, VkExtent2D extent, const std::vector<Texture::Config>& textureConfigs, const std::vector<RenderPassConfig>& passConfigs, Swapchain* previousSwapchain);
        void init(const std::vector<Texture::Config>& textureConfigs, const std::vector<RenderPassConfig>& _passConfigs);
        Swapchain(const Swapchain& copy) = delete;
        Swapchain& operator=(const Swapchain& copy) = delete;
        Swapchain(Swapchain&& move) = delete;
        Swapchain& operator=(Swapchain&& move) = delete;
        ~Swapchain();

        unsigned int getImageCount() { return imageCount; };
        VkResult acquireNextImage(unsigned int* imageIndex);
        void waitForCommandBuffer();
        VkResult submitCommandBuffers(const VkCommandBuffer* buffers, unsigned int* imageIndex);
        bool swapchainsCompatible(const Swapchain& otherSwapchain) const;
    private:
        Device* device;
        VkExtent2D windowExtent;
        VkSwapchainKHR swapChain;
        std::vector<std::vector<SubPassConfig>> passConfigs;
        Swapchain* oldSwapchain;

        void createSwapChain();
        void createTextures(std::vector<Texture::Config> textureConfigs, std::vector<RenderPassConfig>& passConfigs);
        void createRenderPass(const unsigned int& renderPassIndex, const RenderPassConfig& passConfig);
        void createRenderPassFrameBuffers(const unsigned int& renderPassIndex, const RenderPassConfig& passConfig);
        void createSyncObjects();

        VkSurfaceFormatKHR chooseSwapSurfaceFormat(
            const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(
            const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
        VkFormat findDepthFormat();

        VkFormat swapChainImageFormat;
        VkFormat swapChainDepthFormat;
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        std::vector<VkFence> imagesInFlight;
    };
}

#endif