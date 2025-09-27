#ifndef ENG_BUFFER
#define ENG_BUFFER

#include "Helpers.h"
#include "Device.h"

namespace Eng {
    class Buffer {
        Device* device;
        VkDeviceSize bufferSize;
        VkBufferUsageFlags bufferUsage;
        VkMemoryPropertyFlags memoryProperties;
        VkDeviceSize minOffsetAlignment;

        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory GPUmemory = VK_NULL_HANDLE;
        bool mappable = false;
        bool mapped = false;
        void* CPUmemory = nullptr;

    public:
        Buffer(Device* _device, const VkDeviceSize& _instanceSize, const unsigned int& _instanceCount, const VkBufferUsageFlags& _bufferUsage, const VkMemoryPropertyFlags& _memoryProperties, const VkDeviceSize& _minOffsetAlignment = 1);
        Buffer(const Buffer& copy) = delete;
        Buffer& operator=(const Buffer& copy) = delete;
        Buffer(Buffer&& move) = default;
        Buffer& operator=(Buffer&& move) = default;
        ~Buffer();

        void map(const VkDeviceSize& size = VK_WHOLE_SIZE, const VkDeviceSize& offset = 0);
        void write(const void* data, const unsigned int& count, const VkDeviceSize& offset = 0);
        VkResult flush(const VkDeviceSize& size = VK_WHOLE_SIZE, const VkDeviceSize& offset = 0);
        VkResult invalidate(const VkDeviceSize& size = VK_WHOLE_SIZE, const VkDeviceSize& offset = 0);
        void unmap();
        void copyToDeviceLocal(const VkBufferUsageFlags& bufferUsage);
        VkDescriptorBufferInfo descriptorInfo(const VkDeviceSize& size = VK_WHOLE_SIZE, const VkDeviceSize& offset = 0);

        void writeAtIndex(const void* data, const unsigned int& index);
        void flushAtIndex(const unsigned int& index);
        void invalidateAtIndex(const unsigned int& index);
        void descriptorInfoForIndex(const unsigned int& index);
        
        unsigned int getOffsetOfIndex(const unsigned int& i) { return i*paddedInstaceSize; };
        VkBuffer getBuffer() { return buffer; };
        
        VkDeviceSize instanceSize;
        VkDeviceSize paddedInstaceSize;
        VkDeviceSize instanceCount;
    };
}

#endif