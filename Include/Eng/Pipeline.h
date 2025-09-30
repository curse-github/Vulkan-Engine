#ifndef ENG_PIPELINE
#define ENG_PIPELINE

#include "Helpers.h"
#include "Device.h"
#include "FrameInfo.h"

// input assembler -> vertex shader -> rasterization -> fragment shader -> color blending
namespace Eng {
    struct PipelineConfig {
        PipelineConfig() = default;
        PipelineConfig(const PipelineConfig& copy) = delete;
        PipelineConfig& operator=(const PipelineConfig& copy) = delete;
        PipelineConfig(PipelineConfig&& move) = delete;
        PipelineConfig& operator=(PipelineConfig&& move) = delete;

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
        std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
        std::vector<VkDynamicState> dynamicStateEnables;
        VkPipelineDynamicStateCreateInfo dynamicStateInfo;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkRenderPass renderPass = VK_NULL_HANDLE;
        unsigned int subpass = 0;

        void setDefaults();
        void enableAlphaBlending();

        static VkPipelineColorBlendAttachmentState defaultColorBlendState() {
            return VkPipelineColorBlendAttachmentState{
                VK_FALSE,
                VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO,
                VK_BLEND_OP_ADD,
                VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO,
                VK_BLEND_OP_ADD,
                VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT
            };
        }
        static VkPipelineColorBlendAttachmentState alphaBlendingColorBlendState() {
            return VkPipelineColorBlendAttachmentState{
                VK_TRUE,
                VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                VK_BLEND_OP_ADD,
                VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO,
                VK_BLEND_OP_ADD,
                VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT
            };
        }
    };
    class Pipeline {
        Device* device;
        VkPipeline graphicsPipeline;
        VkShaderModule vertShaderModule;
        VkShaderModule fragShaderModule;
        public:
        Pipeline(Device* _device, const std::string& vert, const std::string& frag, const PipelineConfig& config);
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