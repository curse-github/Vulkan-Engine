#ifndef ENG_SWAPCHAIN
#define ENG_SWAPCHAIN

#include "Helpers.h"
#include "Device.h"
#include "Texture.h"

namespace Eng {
    class Swapchain {
    public:
        struct TextureConfig {
        public:
            VkFormat format;
            VkImageUsageFlagBits usage;
            VkImageAspectFlagBits aspect;
            VkImageLayout imageLayout;
            VkImageLayout attachmentLayout;
            bool createSampler;
            
            TextureConfig() = default;
            TextureConfig(const TextureConfig& copy) = default;
            TextureConfig& operator=(const TextureConfig& copy) = default;
            TextureConfig(TextureConfig&& move) = default;
            TextureConfig& operator=(TextureConfig&& move) = default;
            ~TextureConfig() = default;

            static TextureConfig createColorImage() {
                return TextureConfig{
                    VK_FORMAT_B8G8R8A8_UNORM,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    false
                };
            };
            static TextureConfig createDepthTexture(const VkFormat& format = VK_FORMAT_D32_SFLOAT) {
                return TextureConfig{
                    format,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    VK_IMAGE_ASPECT_DEPTH_BIT,
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    false
                };
            }
        };
        struct SubPassConfig {
        public:
            static inline constexpr unsigned int NO_DEPTH_ATTACHMENT = ~0u;
            std::vector<unsigned int> inputAttachmentIndexes{};
            std::vector<unsigned int> colorAttachmentIndexes{};
            unsigned int depthAttachmentIndex = NO_DEPTH_ATTACHMENT;

            SubPassConfig() = default;
            SubPassConfig(const SubPassConfig& copy) = default;
            SubPassConfig& operator=(const SubPassConfig& copy) = default;
            SubPassConfig(SubPassConfig&& move) = default;
            SubPassConfig& operator=(SubPassConfig&& move) = default;
            ~SubPassConfig() = default;
        };
        struct SwapchainConfig {
        public:
            std::vector<TextureConfig> textureConfigs{};
            std::vector<std::vector<SubPassConfig>> passConfigs;

            SwapchainConfig(const std::vector<TextureConfig>& _textureConfigs, const std::vector<std::vector<SubPassConfig>>& _passConfigs)
                : passConfigs(_passConfigs)
            {
                textureConfigs.push_back(TextureConfig::createDepthTexture());
                if (_textureConfigs.size() > 0)
                    textureConfigs.insert(textureConfigs.end(), _textureConfigs.begin(), _textureConfigs.end());
            };
            SwapchainConfig(const SwapchainConfig& copy) = default;
            SwapchainConfig& operator=(const SwapchainConfig& copy) = default;
            SwapchainConfig(SwapchainConfig&& move) = default;
            SwapchainConfig& operator=(SwapchainConfig&& move) = default;
            ~SwapchainConfig() = default;
        };

        std::vector<VkImage> swapChainImages;
        std::vector<VkImageView> swapChainImageViews;
        unsigned int currentFrame = 0;
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
        unsigned int imageCount = 0;
        std::vector<std::vector<Texture*>> textures;
        std::vector<std::vector<VkDescriptorImageInfo>> textureDescriptors;
        std::vector<VkAttachmentDescription> textureAttachmentDescriptors;
        std::vector<VkClearValue> clearValues;

        std::vector<VkFramebuffer> swapChainFramebuffers;
        std::vector<VkRenderPass> renderPasses;
        VkExtent2D swapChainExtent;
        SwapchainConfig config;

        Swapchain(Device* _device, VkExtent2D extent, const SwapchainConfig& _config);
        Swapchain(Device* _device, VkExtent2D extent, const SwapchainConfig& _config, Swapchain* previousSwapchain);
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
        Swapchain* oldSwapchain;

        void init();
        void createSwapChain();
        void createImageViews();
        void createTextures();
        void createRenderPass(const unsigned int& renderPassIndex);
        void createRenderPassFrameBuffers(const unsigned int& renderPassIndex);
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