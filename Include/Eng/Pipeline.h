#ifndef ENG_PIPELINE
#define ENG_PIPELINE

#include "Helpers.h"
#include "Device.h"
#include "FrameInfo.h"

// input assembler -> vertex shader -> rasterization -> fragment shader -> color blending
namespace Eng {
    struct PipelineConfigInfo {
        PipelineConfigInfo() = default;
        PipelineConfigInfo(const PipelineConfigInfo& copy) = delete;
        PipelineConfigInfo& operator=(const PipelineConfigInfo& copy) = delete;
        PipelineConfigInfo(PipelineConfigInfo&& move) = delete;
        PipelineConfigInfo& operator=(PipelineConfigInfo&& move) = delete;

        std::vector<VkSpecializationMapEntry> vertSpecializationInfoEntries{};
        std::vector<unsigned char> vertSpecializationInfoData{};
        std::vector<VkSpecializationMapEntry> fragSpecializationInfoEntries{};
        std::vector<unsigned char> fragSpecializationInfoData{};
        void addVertSpecializationConstant(const unsigned int& constant_id, const unsigned int& value);
        void addFragSpecializationConstant(const unsigned int& constant_id, const unsigned int& value);
        std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
        VkPipelineViewportStateCreateInfo viewportInfo;
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
        VkPipelineRasterizationStateCreateInfo rasterizationInfo;
        VkPipelineMultisampleStateCreateInfo multisampleInfo;
        VkPipelineColorBlendAttachmentState colorBlendAttachment;
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
        std::vector<VkDynamicState> dynamicStateEnables;
        VkPipelineDynamicStateCreateInfo dynamicStateInfo;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkRenderPass renderPass = VK_NULL_HANDLE;
        unsigned int subpass = 0;

        void setDefaults();
        void enableAlphaBlending();
    };
    class Pipeline {
        Device* device;
        VkPipeline graphicsPipeline;
        VkShaderModule vertShaderModule;
        VkShaderModule fragShaderModule;
        public:
        Pipeline(Device* _device, const std::string& vert, const std::string& frag, const PipelineConfigInfo& config);
        Pipeline(const Pipeline& copy) = delete;
        Pipeline& operator=(const Pipeline& copy) = delete;
        Pipeline(Pipeline&& move) = delete;
        Pipeline& operator=(Pipeline&& move) = delete;
        ~Pipeline();
        
        void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);
        void bind(VkCommandBuffer commandBuffer);
    };
}

#endif