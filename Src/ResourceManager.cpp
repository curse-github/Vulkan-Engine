#include "ResourceManager.h"
namespace Eng {
    ResourceManager::ResourceManager(Device* _device, DescriptorPool* _globalDescriptorPool, const unsigned int& _maxTextures, const unsigned int& _minUniformBufferOffsetAlignment)
        : device(_device), globalDescriptorPool(_globalDescriptorPool), maxTextures(_maxTextures), minUniformBufferOffsetAlignment(_minUniformBufferOffsetAlignment)
    {
        textureIdxs[""] = storeTexture("Resources/Textures/color/White.bmp");
        storeTexture("Resources/Textures/normal/Normal.bmp");
        mappedUniforms.reserve(25);
    }
    ResourceManager::~ResourceManager() {

    }
    
    Mesh* ResourceManager::getMesh(const std::string& mesh) {
        if (meshes.count(mesh) == 0) meshes[mesh] = Loaders::MeshLoader::fromObj(device, mesh);
        return meshes[mesh].value;
    }
    unsigned int ResourceManager::getMaterialIdx(const std::string& mtlFile, const std::string& materialName) {
        if (loadedMtls.count(mtlFile) == 0) {
            loadedMtls[mtlFile] = 1;
            Loaders::MaterialLoader::fromMtl(device, mtlFile, this);
        }
        return materialIdxs[mtlFile+materialName];
    }
    unsigned int ResourceManager::storeTexture(const std::string& texture) {
        if (textureIdxs.count(texture) == 0) {
            if (textures.size() == maxTextures)
                throw std::runtime_error("Tried to load too many textures!");
            textureIdxs[texture] = textures.size();
            textures.push_back(Loaders::TextureLoader::fromBmp(device, texture));
            numTextures = textures.size();
        }
        return textureIdxs[texture];
    }
    unsigned int ResourceManager::storeMaterial(const std::string& materialName, const MaterialUboData& data) {
        materialIdxs[materialName] = materials.size();
        materials.push_back(data);
        numMaterials = materials.size();
        return materialIdxs[materialName];
    }
    void ResourceManager::createMaterialUniform() {
        materialUniformBuffer = new Buffer(device, sizeof(MaterialUboData), materials.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, minUniformBufferOffsetAlignment);
        materialUniformBuffer->map();
        materialUniformBuffer->write(materials.data(), materials.size());
        materialUniformBuffer->flush();
        materialUniformBuffer->unmap();
        materialUniformBuffer->copyToDeviceLocal(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        // create uniform buffer descriptor set layouts
        materialDescriptorSetLayout = DescriptorSetLayout::Builder(device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, textures.size())
            .addBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT).build();
        // populate uniform buffer descriptor sets with descriptors
        //     texture descriptors
        std::vector<VkDescriptorImageInfo> textureDescriptors(textures.size());
        for (size_t i = 0; i < textures.size(); i++) textureDescriptors[i] = textures[i]->descriptorInfo();
        //     material descriptor
        VkDescriptorBufferInfo materialUniformBufferDescriptor = materialUniformBuffer->descriptorInfo();
        if (
            !(DescriptorWriter(materialDescriptorSetLayout.value, globalDescriptorPool)
            .writeImages(0, textureDescriptors.data(), textureDescriptors.size())
            .writeBuffer(1, &materialUniformBufferDescriptor).build(materialDescriptorSet))
        )
            std::cout << "Building material descriptor set failed.\n";
    }

    ResourceManager::MappedUniformData* ResourceManager::getMappedUniform(
        const VkDeviceSize &bufferCount, const VkDeviceSize &instanceSize, const unsigned int &instanceCount,
        const VkDescriptorType& type, const VkShaderStageFlags& stages
    ) {
        MappedUniformData mappedUniform{};
        mappedUniform.setLayout = DescriptorSetLayout::Builder(device)
            .addBinding(0, type, stages, 1)
            .build();
        mappedUniform.buffers.resize(bufferCount);
        mappedUniform.sets.resize(bufferCount);
        for (size_t i = 0; i < bufferCount; i++) {
            mappedUniform.buffers[i] = new Buffer(device, instanceSize, instanceCount, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, device->properties.limits.minUniformBufferOffsetAlignment);
            mappedUniform.buffers[i]->map();
            VkDescriptorBufferInfo bufferDescriptor = mappedUniform.buffers[i]->descriptorInfo((type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) ? mappedUniform.buffers[i]->paddedInstaceSize : ~0ull);
            if (!DescriptorWriter(mappedUniform.setLayout, globalDescriptorPool).writeBuffer(0, &bufferDescriptor).build(mappedUniform.sets[i]))
                std::cout << "Building ubo descriptor set failed.\n";
        }
        size_t index = mappedUniforms.size();
        mappedUniforms.push_back((MappedUniformData&&)mappedUniform);
        return &mappedUniforms[index];
    }
    void ResourceManager::multiplyMaterialNorm(const std::string& materialFile, const std::string& material, const float& normMult) {
        float initial = materials[materialIdxs[materialFile+material]].normMult.w;
        materials[materialIdxs[materialFile+material]].normMult = {normMult, normMult, 1.0f, initial};
    }
}