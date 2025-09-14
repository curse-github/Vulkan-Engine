#include "Descriptors.h"

namespace Eng {
    DescriptorSetLayout::Builder::Builder(Device* _device) : device(_device) {}
    DescriptorSetLayout::Builder& DescriptorSetLayout::Builder::addBinding(const unsigned int& binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, const unsigned int& count) {
        assert(bindings.count(binding) == 0 && "Binding already in use");
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = binding;
        layoutBinding.descriptorType = descriptorType;
        layoutBinding.descriptorCount = count;
        layoutBinding.stageFlags = stageFlags;
        bindings[binding] = layoutBinding;
        return *this;
    }
    DescriptorSetLayout* DescriptorSetLayout::Builder::build() const {
        return new DescriptorSetLayout(device, bindings);
    }


    DescriptorSetLayout::DescriptorSetLayout(Device* _device, std::unordered_map<unsigned int, VkDescriptorSetLayoutBinding> bindings) : device(_device), bindings(bindings) {
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
        for (const std::pair<unsigned int, VkDescriptorSetLayoutBinding>& kv : bindings)
            setLayoutBindings.push_back(kv.second);
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
        descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutInfo.bindingCount = static_cast<unsigned int>(setLayoutBindings.size());
        descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();
        if (vkCreateDescriptorSetLayout( device->device,& descriptorSetLayoutInfo, nullptr,& descriptorSetLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create descriptor set layout!");
    }
    DescriptorSetLayout::~DescriptorSetLayout() {
        vkDestroyDescriptorSetLayout(device->device, descriptorSetLayout, nullptr);
    }




    DescriptorPool::Builder::Builder(Device* _device) : device(_device) {}
    DescriptorPool::Builder& DescriptorPool::Builder::addPoolSize(VkDescriptorType descriptorType, const unsigned int& count) {
        poolSizes.push_back(VkDescriptorPoolSize{descriptorType, count});
        return *this;
    }
    DescriptorPool::Builder& DescriptorPool::Builder::setPoolFlags(VkDescriptorPoolCreateFlags flags) {
        poolFlags = flags;
        return *this;
    }
    DescriptorPool::Builder& DescriptorPool::Builder::setMaxSets(const unsigned int& count) {
        maxSets = count;
        return *this;
    }
    DescriptorPool* DescriptorPool::Builder::build() const {
        return new DescriptorPool(device, maxSets, poolFlags, poolSizes);
    }


    DescriptorPool::DescriptorPool(Device* _device, const unsigned int& maxSets, VkDescriptorPoolCreateFlags poolFlags, const std::vector<VkDescriptorPoolSize>& poolSizes) : device(_device) {
        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.poolSizeCount = static_cast<unsigned int>(poolSizes.size());
        descriptorPoolInfo.pPoolSizes = poolSizes.data();
        descriptorPoolInfo.maxSets = maxSets;
        descriptorPoolInfo.flags = poolFlags;
        if (vkCreateDescriptorPool(device->device,& descriptorPoolInfo, nullptr,& descriptorPool) != VK_SUCCESS)
            throw std::runtime_error("Failed to create descriptor pool!");
    }
    DescriptorPool::~DescriptorPool() {
        vkDestroyDescriptorPool(device->device, descriptorPool, nullptr);
    }
    bool DescriptorPool::allocateDescriptorSet(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.pSetLayouts =& descriptorSetLayout;
        allocInfo.descriptorSetCount = 1;
        // Might want to create a "DescriptorPoolManager" class that handles this case, and builds
        // a new pool whenever an old pool fills up. But this is beyond our current scope
        if (vkAllocateDescriptorSets(device->device,& allocInfo,& descriptor) != VK_SUCCESS)
            return false;
        return true;
    }
    void DescriptorPool::freeDescriptorSet(std::vector<VkDescriptorSet>& descriptors) const {
        vkFreeDescriptorSets(device->device, descriptorPool, static_cast<unsigned int>(descriptors.size()), descriptors.data());
    }
    void DescriptorPool::resetPool() {
        vkResetDescriptorPool(device->device, descriptorPool, 0);
    }




    DescriptorWriter::DescriptorWriter(DescriptorSetLayout* _setLayout, DescriptorPool* _pool) : setLayout(_setLayout), pool(_pool) {}
    DescriptorWriter& DescriptorWriter::writeBuffer(const unsigned int& binding, VkDescriptorBufferInfo *bufferInfo) {
        assert(setLayout->bindings.count(binding) == 1 && "Layout does not contain specified binding.");
        VkDescriptorSetLayoutBinding& bindingDescription = setLayout->bindings[binding];
        assert(bindingDescription.descriptorCount == 1 && "Binding single descriptor info, but binding expects multiple.");
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.pBufferInfo = bufferInfo;
        write.descriptorCount = 1;
        
        writes.push_back(write);
        return *this;
    }
    DescriptorWriter& DescriptorWriter::writeImage(const unsigned int& binding, VkDescriptorImageInfo *imageInfo) {
        assert(setLayout->bindings.count(binding) == 1 && "Layout does not contain specified binding.");
        VkDescriptorSetLayoutBinding& bindingDescription = setLayout->bindings[binding];
        assert(bindingDescription.descriptorCount == 1 && "Binding single descriptor info, but binding expects multiple.");
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.pImageInfo = imageInfo;
        write.descriptorCount = 1;
        writes.push_back(write);
        return *this;
    }
    DescriptorWriter& DescriptorWriter::writeImages(const unsigned int& binding, VkDescriptorImageInfo *imageInfo, const size_t& count) {
        assert(setLayout->bindings.count(binding) == 1 && "Layout does not contain specified binding");
        VkDescriptorSetLayoutBinding& bindingDescription = setLayout->bindings[binding];
        assert(bindingDescription.descriptorCount == count && "Binding incorrect number of descriptor infos.");
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.pImageInfo = imageInfo;
        write.descriptorCount = count;
        writes.push_back(write);
        return *this;
    }
    bool DescriptorWriter::build(VkDescriptorSet& set) {
        bool success = pool->allocateDescriptorSet(setLayout->descriptorSetLayout, set);
        if (!success) return false;
        overwrite(set);
        return true;
    }
    void DescriptorWriter::overwrite(VkDescriptorSet& set) {
        for (auto& write : writes)
            write.dstSet = set;
        vkUpdateDescriptorSets(pool->device->device, writes.size(), writes.data(), 0, nullptr);
    }
}