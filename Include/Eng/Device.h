#ifndef ENG_DEVICE
#define ENG_DEVICE

#if defined(_DEBUG) && (_DEBUG==1) && defined(_WINDOWS)
#define VALIDATION_LAYERS
#endif

#include "Helpers.h"
#include "Window.h"

namespace Eng {
    // https://drive.google.com/drive/folders/1Hs-3v_AFVbASmymY4I2UB-JWvW3-hTAV
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
    struct QueueFamilyIndices {
        unsigned int graphicsFamily;
        unsigned int presentFamily;
        bool graphicsFamilyHasValue = false;
        bool presentFamilyHasValue = false;
        bool isComplete() { return graphicsFamilyHasValue && presentFamilyHasValue; }
    };
    class Device {
        Window* window;
        const std::vector<const char* > validationLayers = {"VK_LAYER_KHRONOS_validation"};
        const std::vector<const char* > deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
#if defined(_LINUX)
        const std::vector<const char* > instanceExtensions = {};
#else
        const std::vector<const char* > instanceExtensions = {"VK_NV_external_memory_capabilities"};// not needed, and will cause errors on linux
#endif
#ifdef VALIDATION_LAYERS
        VkDebugUtilsMessengerEXT debugMessenger;
        bool checkValidationLayerSupport();
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
#endif
    public:
        VkInstance instance;
        VkQueue graphicsQueue;
        VkQueue presentQueue;
        VkDevice device;
        VkSurfaceKHR surface;
        VkCommandPool commandPool;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties properties;
        Device(Window* _window);
        Device(const Device& copy) = delete;
        Device& operator=(const Device& copy) = delete;
        Device(Device&& move) = delete;
        Device& operator=(Device&& move) = delete;
        ~Device();
        
        Eng::SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(physicalDevice); };
        QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(physicalDevice); };
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryProperties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        void createImage(const unsigned int& width, const unsigned int& height, const VkFormat& format, const VkImageTiling& tiling, const VkImageUsageFlags& imageUsage, const VkMemoryPropertyFlags& memoryProperties, VkImage& image, VkDeviceMemory& imageMemory);
        void transitionImageLayout(VkImage image, const VkImageAspectFlags& aspect, const VkImageLayout& from, const VkImageLayout& to);
        void copyBufferToImage(const VkBuffer& buffer, const VkImage& image, const VkImageAspectFlags& aspect, const unsigned int& width, const unsigned int& height);
        VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);// used in swapchain
    private:

        void createInstance();
        void createSurface();
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createCommandPool();

        std::vector<const char* > getRequiredExtensions();
        void hasGflwRequiredInstanceExtensions();
        bool isDeviceSuitable(VkPhysicalDevice device);
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

        bool checkDeviceExtensionSupport(VkPhysicalDevice device);
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

        // only used by buffer functions
        unsigned int findMemoryType(unsigned int typeFilter, VkMemoryPropertyFlags properties);
        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    };
}

#endif