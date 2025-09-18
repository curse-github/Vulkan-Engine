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
        Texture(Device* _device, const unsigned int& _width, const unsigned int& _height, const void* data, const VkFormat& format = VK_FORMAT_R8G8B8A8_UNORM, const VkImageTiling& tiling = VK_IMAGE_TILING_LINEAR, const VkImageUsageFlags& imageUsage = VK_IMAGE_USAGE_SAMPLED_BIT, const VkImageAspectFlags& aspect = VK_IMAGE_ASPECT_COLOR_BIT, const VkImageLayout& layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, const bool& createSampler = true);
        Texture(const Texture& copy) = delete;
        Texture& operator=(const Texture& copy) = delete;
        Texture(Texture&& move) = default;
        Texture& operator=(Texture&& move) = default;
        ~Texture();

        unsigned int width;
        unsigned int height;
        
        VkImageView getView() { return view; }
        VkDescriptorImageInfo descriptorInfo();
    };
}

#endif