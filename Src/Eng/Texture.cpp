#include "Texture.h"

namespace Eng {
    Texture::Texture(
        Device* _device, const unsigned int& _width, const unsigned int& _height, const void* data,
        const VkFormat& format, const VkImageTiling& tiling, const VkImageUsageFlags& imageUsage, const VkImageAspectFlags& _aspect,
        const VkImageLayout& _layout, const bool& createSampler, const bool& unnormalizedCoordinates
    ) : device(_device), width(_width), height(_height), aspect(_aspect), layout(_layout) {
        VkFilter filter = (unnormalizedCoordinates == VK_FALSE) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
        VkSamplerAddressMode addressMode = (unnormalizedCoordinates == VK_FALSE) ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;

        if (data != nullptr) {
            unsigned int size = width*height;
            Buffer stagingBuffer(device, 4, width*height, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 0);
            stagingBuffer.map();
            stagingBuffer.write(data, width*height);
            stagingBuffer.unmap();
            device->createImage(width, height, format, tiling, VK_IMAGE_USAGE_TRANSFER_DST_BIT | imageUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, GPUmemory);
            device->transitionImageLayout(image, aspect, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            device->copyBufferToImage(stagingBuffer.getBuffer(), image, aspect, width, height);
            device->transitionImageLayout(image, aspect, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layout);
        } else {
            device->createImage(width, height, format, tiling, imageUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, GPUmemory);
            if (layout != VK_IMAGE_LAYOUT_UNDEFINED) device->transitionImageLayout(image, aspect, VK_IMAGE_LAYOUT_UNDEFINED, layout);
        }
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = aspect;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device->device, &viewInfo, nullptr, &view) != VK_SUCCESS)
            throw std::runtime_error("Failed to create image view!");
        if (createSampler) {
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = filter;
            samplerInfo.minFilter = filter;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            samplerInfo.addressModeU = addressMode;
            samplerInfo.addressModeV = addressMode;
            samplerInfo.addressModeW = addressMode;
            samplerInfo.mipLodBias = 0.0f;
            samplerInfo.anisotropyEnable = VK_FALSE;
            samplerInfo.maxAnisotropy = 1u;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;// optional, since campare is disabled
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = 0.0f;
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = unnormalizedCoordinates;
            if (vkCreateSampler(device->device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
                throw std::runtime_error("Failed to create sampler!");
        }
    }
    Texture::~Texture() {
        if (sampler != VK_NULL_HANDLE) vkDestroySampler(device->device, sampler, nullptr);
        vkDestroyImageView(device->device, view, nullptr);
        vkDestroyImage(device->device, image, nullptr);
        vkFreeMemory(device->device, GPUmemory, nullptr);
    }
    
    VkDescriptorImageInfo Texture::descriptorInfo() {
        return VkDescriptorImageInfo{
            sampler,
            view,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
    }
    void Texture::transitionLayout(const VkImageLayout& from, const VkImageLayout& to) {
        device->transitionImageLayout(image, aspect, from, to);
        layout = to;
    }
    void Texture::transitionLayout(const VkImageLayout& _layout) {
        if (layout == _layout) return;
        device->transitionImageLayout(image, aspect, layout, _layout);
        layout = _layout;
    }
}