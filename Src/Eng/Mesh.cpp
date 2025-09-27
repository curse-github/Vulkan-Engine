#include "Mesh.h"

namespace Eng {
    Mesh::Mesh(Device* _device, Mesh::MeshData&& meshData) : device(_device) {
        // create vertex buffer
        vertexCount = static_cast<unsigned int>(meshData.vertices.size());
        assert((meshData.vertices.size() >= 3) && "Vertex count must be at least 3");
        vertexBuffer = new Buffer(device, sizeof(meshData.vertices[0]), vertexCount, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 0);
        vertexBuffer->map();
        vertexBuffer->write(meshData.vertices.data(), static_cast<unsigned int>(meshData.vertices.size()), 0);
        vertexBuffer->unmap();
        vertexBuffer->copyToDeviceLocal(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        // create index buffer
        indexCount = static_cast<unsigned int>(meshData.indices.size());
        hasIndexBuffer = indexCount > 0;
        if (!hasIndexBuffer) return;
        assert((meshData.indices.size() >= 3) && "Index count must be either 0, or least 3");
        indexBuffer = new Buffer(device, sizeof(meshData.indices[0]), indexCount, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 0);
        indexBuffer->map();
        indexBuffer->write(meshData.indices.data(), static_cast<unsigned int>(meshData.indices.size()), 0);
        indexBuffer->unmap();
        indexBuffer->copyToDeviceLocal(VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    }
    // https://en.wikipedia.org/wiki/Wavefront_.obj_file
    Mesh::~Mesh() {
        delete vertexBuffer;
        if (indexBuffer != nullptr) delete indexBuffer;
    }
    
    std::vector<VkVertexInputBindingDescription> Mesh::Vertex::getBindingDescriptions() {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = sizeof(Vertex);
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescriptions;
    }
    std::vector<VkVertexInputAttributeDescription> Mesh::Vertex::getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
        // "VK_FORMAT_R32_SFLOAT" is 1 floats
        // "VK_FORMAT_R32G32_SFLOAT" is 2 floats
        // "VK_FORMAT_R32G32B32_SFLOAT" is 3 floats
        // "VK_FORMAT_R32G32B32A32_SFLOAT" is 4 floats
        attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
        attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32_SFLOAT   , offsetof(Vertex, uv)});
        attributeDescriptions.push_back({2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});
        attributeDescriptions.push_back({3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, tangent)});
        return attributeDescriptions;
    }

    void Mesh::draw(VkCommandBuffer commandBuffer) {
        VkBuffer buffers[] = { vertexBuffer->getBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
        if (hasIndexBuffer) {
            vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
        } else
            vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
    }
};