#ifndef __RESOURCEMANAGER
#define __RESOURCEMANAGER

#include "Helpers.h"
#include "Device.h"
#include "Descriptors.h"
#include "Loaders.h"
#include "Buffer.h"
#include "UboStructs.h"

namespace Eng {
    class ResourceManager {
        Device* device;
        DescriptorPool* globalDescriptorPool;

        unsigned int maxTextures;
        std::unordered_map<std::string, OwnedPointer<Mesh>> meshes;
        std::vector<OwnedPointer<Texture>> textures;
        std::unordered_map<std::string, size_t> textureIdxs;
        std::unordered_map<std::string, unsigned int> loadedMtls;
        std::vector<MaterialUboData> materials;
        std::unordered_map<std::string, size_t> materialIdxs;

        unsigned int minUniformBufferOffsetAlignment;
    public:
        struct MappedUniformData {
            std::vector<OwnedPointer<Buffer>> buffers{};
            OwnedPointer<DescriptorSetLayout> setLayout = nullptr;
            std::vector<VkDescriptorSet> sets{};

            MappedUniformData() = default;
            MappedUniformData(const MappedUniformData& copy) = delete;
            MappedUniformData& operator=(const MappedUniformData& copy) = delete;
            MappedUniformData(MappedUniformData&& move)
                : buffers((std::vector<OwnedPointer<Buffer>>&&)move.buffers), setLayout((OwnedPointer<DescriptorSetLayout>&&)move.setLayout), sets((std::vector<VkDescriptorSet>&&)move.sets)
            {}
            MappedUniformData& operator=(MappedUniformData&& move) = delete;
            ~MappedUniformData() = default;
        };

        ResourceManager(Device* _device, DescriptorPool* _globalDescriptorPool, const unsigned int& _maxTextures, const unsigned int& _minUniformBufferOffsetAlignment);
        ResourceManager(const ResourceManager& copy) = delete;
        ResourceManager& operator=(const ResourceManager& copy) = delete;
        ResourceManager(ResourceManager&& move);
        ResourceManager& operator=(ResourceManager&& move) = delete;
        ~ResourceManager();

        unsigned int numTextures = 0;
        unsigned int numMaterials = 0;
        OwnedPointer<Buffer> materialUniformBuffer;
        OwnedPointer<DescriptorSetLayout> materialDescriptorSetLayout;
        VkDescriptorSet materialDescriptorSet;
        std::vector<MappedUniformData> mappedUniforms;


        Mesh* getMesh(const std::string& mesh);
        void loadMtlFile(const std::string& mtlFile);
        unsigned int getMaterialIdx(const std::string& materialName);
        unsigned int storeTexture(const std::string& texture);
        unsigned int storeMaterial(const std::string& materialName, const MaterialUboData& data);
        
        MappedUniformData* getMappedUniform(
            const VkDeviceSize &bufferCount, const VkDeviceSize &instanceSize, const unsigned int &instanceCount, const VkDescriptorType& type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            const VkShaderStageFlags& stages = VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT, const bool& isCoherent = false
        );
        void createMaterialUniform();
    };
}

#endif// __RESOURCEMANAGER