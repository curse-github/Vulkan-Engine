#include "Buffer.h"
namespace Eng {
    VkDeviceSize getAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment) {
        if (minOffsetAlignment > 0)
            return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
        return instanceSize;
    }
    Buffer::Buffer(Device* _device, const VkDeviceSize& _instanceSize, const unsigned int& _instanceCount, const VkBufferUsageFlags& _bufferUsage, const VkMemoryPropertyFlags& _memoryProperties, const VkDeviceSize& _minOffsetAlignment) :
        device(_device), bufferUsage(_bufferUsage), memoryProperties(_memoryProperties), minOffsetAlignment(_minOffsetAlignment), instanceSize(_instanceSize), instanceCount(_instanceCount)
    {
        paddedInstaceSize = getAlignment(instanceSize, minOffsetAlignment);
        bufferSize = paddedInstaceSize*instanceCount;
        device->createBuffer(bufferSize, bufferUsage, memoryProperties, buffer, GPUmemory);
    }
    Buffer::~Buffer() {
        if (mapped) unmap();
        vkDestroyBuffer(device->device, buffer, nullptr);
        vkFreeMemory(device->device, GPUmemory, nullptr);
    }
    void Buffer::map(const VkDeviceSize& size, const VkDeviceSize& offset) {
        assert((buffer != VK_NULL_HANDLE) && (GPUmemory != VK_NULL_HANDLE) && "cannot map memory which has not been allocated.");
        vkMapMemory(device->device, GPUmemory, offset, size, 0, &CPUmemory);
        mapped = true;
    }
    void Buffer::write(const void* data, const unsigned int& count, const VkDeviceSize& offset) {
        assert(mapped && "cannot write to an unmapped buffer.");
        if (count == VK_WHOLE_SIZE)
            memcpy(((char*)CPUmemory) + offset, data, bufferSize - offset);
        else 
            memcpy(((char*)CPUmemory) + offset, data, count*instanceSize);
    }
    VkResult Buffer::flush(const VkDeviceSize& size, const VkDeviceSize& offset) {
        VkMappedMemoryRange mappedRange{};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = GPUmemory;
        mappedRange.offset = offset;
        mappedRange.size = size;
        return vkFlushMappedMemoryRanges(device->device, 1, &mappedRange);
    }
    VkResult Buffer::invalidate(const VkDeviceSize& size, const VkDeviceSize& offset) {
        VkMappedMemoryRange mappedRange{};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = GPUmemory;
        mappedRange.offset = offset;
        mappedRange.size = size;
        return vkInvalidateMappedMemoryRanges(device->device, 1, &mappedRange);
    }
    void Buffer::unmap() {
        assert(mapped && "cannot unmap memory which hasnt been mapped.");
        vkUnmapMemory(device->device, GPUmemory);
        mapped = false;
    }
    VkDescriptorBufferInfo Buffer::descriptorInfo(const VkDeviceSize& size, const VkDeviceSize& offset) {
        return VkDescriptorBufferInfo{
            buffer,
            offset,
            size,
        };
    }
    void Buffer::copyToDeviceLocal(const VkBufferUsageFlags& newBufferUsage) {
        assert(((bufferUsage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT) != 0) && "cannot copy a buffer which is not a transfer source.");
        VkBuffer stagingBuffer = buffer;
        VkDeviceMemory stagingGPUmemory = GPUmemory;
        buffer = VK_NULL_HANDLE;
        GPUmemory = VK_NULL_HANDLE;
        // move vertex data into GPU local memory, from the coherent memory
        device->createBuffer(bufferSize, newBufferUsage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, GPUmemory);
        device->copyBuffer(stagingBuffer, buffer, bufferSize);
        // cleanup GPU coherent memory buffer
        vkDestroyBuffer(device->device, stagingBuffer, nullptr);
        vkFreeMemory(device->device, stagingGPUmemory, nullptr);
    };


    void Buffer::writeAtIndex(const void* data, const unsigned int& index) {
        write(data, 1, index*paddedInstaceSize);
    }
    void Buffer::flushAtIndex(const unsigned int& index) {
        flush(paddedInstaceSize, index*paddedInstaceSize);
    }
    void Buffer::invalidateAtIndex(const unsigned int& index) {
        invalidate(paddedInstaceSize, index*paddedInstaceSize);
    }
    void Buffer::descriptorInfoForIndex(const unsigned int& index) {
        descriptorInfo(paddedInstaceSize, index*paddedInstaceSize);
    }
}