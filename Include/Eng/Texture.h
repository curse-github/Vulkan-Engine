#ifndef __TEXTURE
#define __TEXTURE

#include "Helpers.h"
#include "Device.h"
#include "Buffer.h"

namespace Eng {
    class Texture {
        Device* device;
        VkSampler sampler = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory GPUmemory = VK_NULL_HANDLE;
    public:
        struct Config {
        public:
            VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
            VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT;
            VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
            VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            bool createSampler = true;
            bool unnormalizedCoordinates = false;
            
            Config() = default;
            Config(const Config& copy) = default;
            Config& operator=(const Config& copy) = default;
            Config(Config&& move) = default;
            Config& operator=(Config&& move) = default;
            ~Config() = default;
            static Config createColorImage();
            static Config createDepthTexture();
        };
        Texture(
            Device* _device, const unsigned int& _width, const unsigned int& _height, const void* data, const VkImageTiling& tiling, const Config& config
        );
        Texture(
            Device* _device, const unsigned int& _width, const unsigned int& _height, const void* data, const VkFormat& format = VK_FORMAT_R8G8B8A8_UNORM,
            const VkImageTiling& tiling = VK_IMAGE_TILING_LINEAR, const VkImageUsageFlags& imageUsage = VK_IMAGE_USAGE_SAMPLED_BIT,
            const VkImageAspectFlags& _aspect = VK_IMAGE_ASPECT_COLOR_BIT, const VkImageLayout& _layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, const bool& createSampler = true, const bool& unnormalizedCoordinates = VK_FALSE
        );
        Texture(const Texture& copy) = delete;
        Texture& operator=(const Texture& copy) = delete;
        Texture(Texture&& move) = default;
        Texture& operator=(Texture&& move) = default;
        ~Texture();

        unsigned int width;
        unsigned int height;
        VkImageAspectFlags aspect;
        VkImageLayout layout;
        
        VkSampler getSampler() { return sampler; }
        VkImageView getView() { return view; }
        VkDescriptorImageInfo descriptorInfo();
        void transitionLayout(const VkImageLayout& _layout, VkCommandBuffer commandBuffer);
    };
}

#endif