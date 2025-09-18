#ifndef ENG_SWAPCHAIN
#define ENG_SWAPCHAIN

#include "Helpers.h"
#include "Device.h"
#include "Texture.h"

namespace Eng {
    // https://drive.google.com/drive/folders/1X0wUwujKw8OH2Z18OENadX0XBKWfU50w
    class Swapchain {
        Device* device;
        VkExtent2D windowExtent;
        VkSwapchainKHR swapChain;
        Swapchain* oldSwapchain;
    public:
        std::vector<Texture*> intermediateColorTextures;
        std::vector<Texture*> depthTextures;
        std::vector<VkImage> swapChainImages;
        std::vector<VkImageView> swapChainImageViews;
        unsigned int currentFrame = 0;
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

        std::vector<VkFramebuffer> swapChainFramebuffers;
        VkRenderPass renderPass;
        VkExtent2D swapChainExtent;

        Swapchain(Device* _device, VkExtent2D extent);
        Swapchain(Device* _device, VkExtent2D extent, Swapchain* previousSwapchain);
        Swapchain(const Swapchain& copy) = delete;
        Swapchain& operator=(const Swapchain& copy) = delete;
        Swapchain(Swapchain&& move) = delete;
        Swapchain& operator=(Swapchain&& move) = delete;
        ~Swapchain();

        VkResult acquireNextImage(unsigned int* imageIndex);
        void waitForCommandBuffer();
        VkResult submitCommandBuffers(const VkCommandBuffer* buffers, unsigned int* imageIndex);
        bool swapchainsCompatible(const Swapchain& otherSwapchain) const;
    private:
        void init();
        void createSwapChain();
        void createImageViews();
        void createRenderPass();
        void createTextures();
        void createFramebuffers();
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