#include "ResourceManager.h"
#include "Renderers.h"

namespace Eng {
    ResourceManager::ResourceManager(Device* _device, EntitySystem* _entitySystem, const unsigned int& _maxTextures, const unsigned int& _minUniformBufferOffsetAlignment)
        : device(_device), entitySystem(_entitySystem), maxTextures(_maxTextures), minUniformBufferOffsetAlignment(_minUniformBufferOffsetAlignment)
    {
        globalDescriptorPool = DescriptorPool::Builder(device)
            .setMaxSets(35+maxTextures)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 25)
            .addPoolSize(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 5)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 5)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxTextures)
            .build();
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
    void ResourceManager::loadMtlFile(const std::string& mtlFile) {
        if (loadedMtls.count(mtlFile) == 0) {
            loadedMtls[mtlFile] = 1;
            Loaders::MaterialLoader::fromMtl(device, mtlFile, this);
        }
    }
    unsigned int ResourceManager::getMaterialIdx(const std::string& materialName) {
        if (materialIdxs.count(materialName) == 0) {
            if (materials.count(materialName) == 0) throw std::runtime_error("material cannot be found.");
            materialIdxs[materialName] = materialBufferData.size();
            materialBufferData.push_back(materials[materialName]);
            updateMaterialUniform();
        }
        return materialIdxs[materialName];
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
    void ResourceManager::storeMaterial(const std::string& materialName, const MaterialUboData& data) {
        materials[materialName] = data;
    }
    ResourceManager::MappedUniformData* ResourceManager::getMappedUniform(
        const VkDeviceSize &bufferCount, const VkDeviceSize &instanceSize, const unsigned int &instanceCount,
        const VkDescriptorType& type, const VkShaderStageFlags& stages, const bool& isCoherent
    ) {
        MappedUniformData mappedUniform{};
        mappedUniform.setLayout = DescriptorSetLayout::Builder(device)
            .addBinding(0, type, stages, 1)
            .build();
        mappedUniform.buffers.resize(bufferCount);
        mappedUniform.sets.resize(bufferCount);
        for (size_t i = 0; i < bufferCount; i++) {
            mappedUniform.buffers[i] = new Buffer(device, instanceSize, instanceCount, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|(isCoherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0u), device->properties.limits.minUniformBufferOffsetAlignment);
            mappedUniform.buffers[i]->map();
            VkDescriptorBufferInfo bufferDescriptor = mappedUniform.buffers[i]->descriptorInfo((type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) ? mappedUniform.buffers[i]->paddedInstaceSize : ~0ull);
            if (!DescriptorWriter(mappedUniform.setLayout, globalDescriptorPool).writeBuffer(0, &bufferDescriptor).build(mappedUniform.sets[i]))
                std::cout << "Building ubo descriptor set failed.\n";
        }
        size_t index = mappedUniforms.size();
        mappedUniforms.push_back((MappedUniformData&&)mappedUniform);
        return &mappedUniforms[index];
    }

    
    void ResourceManager::createMaterialUniform() {
        std::vector<ECS_id_t>& meshRendereredEntityIds = entitySystem->GetEntitiesWithComponent<MeshRendererComponent>();
        materialIdxs.clear();
        materialBufferData.clear();
        numMaterials = materials.size();
        for (size_t i = 0; i < meshRendereredEntityIds.size(); i++) {
            Entity& entity = entitySystem->GetEntity(meshRendereredEntityIds[i]);
            MeshRendererComponent& meshRenderer = entity.GetComponent<MeshRendererComponent>();
            if (materialIdxs.count(meshRenderer.material) == 0) {
                materialIdxs[meshRenderer.material] = materialBufferData.size();
                materialBufferData.push_back(materials[meshRenderer.material]);
            }
        }
        std::cout << "material buffer holds " << materialBufferData.size() << " materials.\n";
        materialUniformBuffer = new Buffer(device, sizeof(MaterialUboData), materialBufferData.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, minUniformBufferOffsetAlignment);
        materialUniformBuffer->map();
        materialUniformBuffer->write(materialBufferData.data(), materialBufferData.size());
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
            .writeBuffer(1, &materialUniformBufferDescriptor)
            .build(materialDescriptorSet))
        )
            std::cout << "Building material descriptor set failed.\n";
    }
    void ResourceManager::updateMaterialUniform() {
        std::cout << "material buffer now holds " << materialBufferData.size() << " materials.\n";
        materialUniformBuffer = new Buffer(device, sizeof(MaterialUboData), materialBufferData.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, minUniformBufferOffsetAlignment);
        materialUniformBuffer->map();
        materialUniformBuffer->write(materialBufferData.data(), materialBufferData.size());
        materialUniformBuffer->unmap();
        materialUniformBuffer->copyToDeviceLocal(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        // populate uniform buffer descriptor sets with descriptors
        //     texture descriptors
        std::vector<VkDescriptorImageInfo> textureDescriptors(textures.size());
        for (size_t i = 0; i < textures.size(); i++) textureDescriptors[i] = textures[i]->descriptorInfo();
        //     material descriptor
        VkDescriptorBufferInfo materialUniformBufferDescriptor = materialUniformBuffer->descriptorInfo();
        DescriptorWriter(materialDescriptorSetLayout.value, globalDescriptorPool)
            .writeImages(0, textureDescriptors.data(), textureDescriptors.size())
            .writeBuffer(1, &materialUniformBufferDescriptor)
            .overwrite(materialDescriptorSet);
    }
}