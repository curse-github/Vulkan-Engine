#ifndef __FRAMEINFO
#define __FRAMEINFO

#include "Helpers.h"
#include "ECS.h"

namespace Eng {
    struct FrameInfo {
        unsigned int frameIndex;
        float t;
        float dt;
        VkCommandBuffer commandBuffer;
        Camera* camera;
        VkDescriptorSet globalDescriptorSet;
        VkDescriptorSet materialDescriptorSet;
        EntitySystem* entitySystem;
    };
}

#endif// __FRAMEINFO