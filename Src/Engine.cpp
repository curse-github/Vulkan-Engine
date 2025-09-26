#include "Engine.h"

namespace Eng {
    GlobalUboData uniformBufferElement;

    Engine::Engine(const std::string& windowName, const ivec2& windowSize)
        : window(windowName, windowSize), device(&window)
    {
        maxTextures = std::min(256u, device.properties.limits.maxDescriptorSetSampledImages);
        globalDescriptorPool = DescriptorPool::Builder(&device)
            .setMaxSets(Swapchain::MAX_FRAMES_IN_FLIGHT*2+3+maxTextures)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, Swapchain::MAX_FRAMES_IN_FLIGHT+2)
            .addPoolSize(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, Swapchain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxTextures)
            .build();
        // make sure white, and the normal texture are the first 
        textureIdxs[""] = storeTexture("Resources/Textures/color/White.bmp");
        storeTexture("Resources/Textures/normal/Normal.bmp");
        window.hideCursor();
        entitySystem.RegisterComponent<TransformComponent>();
        entitySystem.RegisterComponent<MeshRendererComponent>();
        entitySystem.RegisterComponent<PointLightComponent>();
    }
    Engine::~Engine() {
    }

    unsigned int Engine::storeTexture(const std::string& texture) {
        if (textureIdxs.count(texture) == 0) {
            if (textures.size() == maxTextures)
                throw std::runtime_error("Tried to load too many textures!");
            textureIdxs[texture] = textures.size();
            textures.push_back(Loaders::TextureLoader::fromBmp(&device, texture));
        }
        return textureIdxs[texture];
    }
    unsigned int Engine::storeMaterial(const std::string& materialName, const MaterialUboData& data) {
        materialIdxs[materialName] = materials.size();
        materials.push_back(data);
        return materialIdxs[materialName];
    }
    ECS_id_t Engine::addMeshRendereredEntity(
        const vec3& position, const vec3& scale, const vec3& rotation, const std::string& mesh,
        const std::string& materialFile, const std::string& material, const float& normMult
    ) {
        if (meshes.count(mesh) == 0) meshes[mesh] = Loaders::MeshLoader::fromObj(&device, mesh);
        if (loadedMtls.count(materialFile) == 0) {
            loadedMtls[materialFile] = 1;
            Loaders::MaterialLoader::fromMtl(&device, materialFile, this);
        }
        float initial = materials[materialIdxs[materialFile+material]].normMult.w;
        materials[materialIdxs[materialFile+material]].normMult = {normMult, normMult, 1.0f, initial};
        Entity* entity = entitySystem.CreateEntity();
        TransformComponent& transform = entity->AddComponent<TransformComponent>(new TransformComponent());
        transform.position = position;
        transform.scale = scale;
        transform.rotation = rotation;
        MeshRendererComponent& meshRenderer = entity->AddComponent<MeshRendererComponent>(new MeshRendererComponent());
        meshRenderer.mesh = meshes[mesh].value;
        meshRenderer.materialIdx = materialIdxs[materialFile+material];
        return entity->id;
    }
    ECS_id_t Engine::addLightEntity(const vec3& position, const float& size, const vec3& color, const float& intensity) {
        assert((uniformBufferElement.numLights <= MAX_LIGHTS) && "Tried to add too many lights");
        // add light to lights map
        Entity* entity = entitySystem.CreateEntity();
        TransformComponent& transform = entity->AddComponent<TransformComponent>(new TransformComponent());
        PointLightComponent& pointLight = entity->AddComponent<PointLightComponent>(new PointLightComponent());
        transform.position = position;
        transform.scale = vec3(size, 0.0f, 0.0f);
        pointLight.colorIntensity = vec4(color, intensity);
        uniformBufferElement.numLights++;
        return entity->id;
    }

    void Engine::setUpdate(UpdateCallbackT _updateCallback) {
        updateCallback = _updateCallback;
    }
    void Engine::start() {
        started = true;
    }
    bool Engine::pollMovement(const float& dt, TransformComponent& transform) {
        if (window.getKeyPressed(keys.pause)) {
            paused = !paused;
            if (paused) window.showCursor();
            else window.hideCursor();
        }
        if (paused) return false;
        bool updated = false;
        vec2 dMouse = window.getMouseChange();
        if (glm::dot(dMouse, dMouse) > std::numeric_limits<float>::epsilon()){
            dvec2 temp = normalize(dMouse)*dt;
            transform.rotation += vec3(sensitivity.y*temp.y, sensitivity.x*temp.x, 0.0f);
            transform.rotation.x = glm::clamp(transform.rotation.x, -DEG90, DEG90);
            transform.rotation.y = glm::mod(transform.rotation.y, DEG360);
            updated = true;
        }
        const vec3 forward = vec3(glm::sin(transform.rotation.y), 0.0f, glm::cos(transform.rotation.y));
        const vec3 right = vec3(forward.z, 0.0f, -forward.x);// alternatively glm::cross(forward, up)
        vec3 movement(0.0f, 0.0f, 0.0f);
        if (window.getKeyHeld(keys.moveForward)) movement += forward;
        if (window.getKeyHeld(keys.moveBackward)) movement -= forward;
        if (window.getKeyHeld(keys.moveRight)) movement += right;
        if (window.getKeyHeld(keys.moveLeft)) movement -= right;
        if (window.getKeyHeld(keys.moveUp)) movement.y -= 1;
        if (window.getKeyHeld(keys.moveDown)) movement.y += 1;
        if (glm::dot(movement, movement) > std::numeric_limits<float>::epsilon()) {
            transform.position += speed*dt*normalize(movement); updated = true;
        }
        return updated;
    }
    void Engine::run() {
#pragma region setting up uniform data
        // create uniform buffers
        std::vector<OwnedPointer<Buffer>> globalUniformBuffers;
        for (unsigned int i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++) {
            globalUniformBuffers.emplace_back(new Buffer(&device, sizeof(GlobalUboData), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, device.properties.limits.minUniformBufferOffsetAlignment));
            globalUniformBuffers[i]->map();
        }
        OwnedPointer<Buffer> materialUniformBuffer = new Buffer(&device, sizeof(MaterialUboData), materials.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, device.properties.limits.minUniformBufferOffsetAlignment);
        materialUniformBuffer->map();
        materialUniformBuffer->write(materials.data(), materials.size());
        materialUniformBuffer->unmap();
        materialUniformBuffer->copyToDeviceLocal(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        // create uniform buffer descriptor set layouts
        OwnedPointer<DescriptorSetLayout> globalDescriptorSetLayout = DescriptorSetLayout::Builder(&device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT).build();
        OwnedPointer<DescriptorSetLayout> materialDescriptorSetLayout = DescriptorSetLayout::Builder(&device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, textures.size())
            .addBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT).build();
        // create uniform buffer descriptor sets
        std::vector<VkDescriptorSet> globalDescriptorSets(Swapchain::MAX_FRAMES_IN_FLIGHT);
        VkDescriptorSet materialDescriptorSet;
        // populate uniform buffer descriptor sets with descriptors
        //     global uniform buffer descriptors
        for (int i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferDescriptor = globalUniformBuffers[i]->descriptorInfo();
            if (!DescriptorWriter(globalDescriptorSetLayout.value, globalDescriptorPool).writeBuffer(0, &bufferDescriptor).build(globalDescriptorSets[i]))
                std::cout << "Building ubo descriptor set failed.\n";
        }
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
#pragma endregion setting up uniform data

        // setup rendering
        OwnedPointer<RenderSystem> renderSystem = new RenderSystem(&window, &device, (std::vector<std::vector<RenderSystem::SubPass>>&&)std::vector<std::vector<RenderSystem::SubPass>>{{
            {{{}, {}, {1}, 2}, std::vector<RendererAbstract*>{
                new DiffuseBlinnPhongRenderer(&device, globalDescriptorSetLayout->descriptorSetLayout, materialDescriptorSetLayout->descriptorSetLayout, textures.size(), materials.size(), globalDescriptorPool),
                new PointLightRenderer(&device, globalDescriptorSetLayout->descriptorSetLayout)
            }},
            {{{}, {1, 2}, {3}, Swapchain::SubPassConfig::NO_DEPTH_ATTACHMENT}, std::vector<RendererAbstract*>{
                new OnTilePostProcessRenderer(&device, globalDescriptorSetLayout->descriptorSetLayout)
            }}
        },{
            {{{3}, {}, {0}, Swapchain::SubPassConfig::NO_DEPTH_ATTACHMENT}, std::vector<RendererAbstract*>{
                new OffTilePostProcessRenderer(&device, globalDescriptorSetLayout->descriptorSetLayout)
            }}
        }}, globalDescriptorPool);
        
        Camera camera;
        TransformComponent viewerTransform;
        viewerTransform.position.z = -2.5f;
        camera.setViewYXZ(viewerTransform.position, viewerTransform.rotation);
        FrameInfo frameInfo(&camera, materialDescriptorSet, &entitySystem);
        while(!window.shouldClose()) {
            glfwPollEvents();
            camera.setProj(glm::radians(50.0f), renderSystem->getAspectRatio(), 0.1f, 100.0f);// must be done since aspect ratio can change.
            frameInfo.commandBuffer = renderSystem->beginFrame();
            if (frameInfo.commandBuffer != VK_NULL_HANDLE) {
                // set frame specific info
                frameInfo.imageIndex = renderSystem->getImage();
                frameInfo.frameIndex = renderSystem->getFrame();
                frameInfo.globalDescriptorSet = globalDescriptorSets[frameInfo.frameIndex];
                frameInfo.updateTime();
                if (pollMovement(glm::min(frameInfo.dt, 1.0f/30.0f), viewerTransform))
                    camera.setViewYXZ(viewerTransform.position, viewerTransform.rotation);
                // let user update things
                if (updateCallback != nullptr) updateCallback(frameInfo);
                // update uniform
                uniformBufferElement.resolution = vec4(renderSystem->getResolution(), 0.0f, 0.0f);
                std::vector<ECS_id_t>& lightsEntityIds = frameInfo.entitySystem->GetEntitiesWithComponent<PointLightComponent>();
                size_t i = 0;
                for (; i < lightsEntityIds.size(); i++) {
                    Entity& entity = frameInfo.entitySystem->GetEntity(lightsEntityIds[i]);
                    TransformComponent& transform = entity.GetComponent<TransformComponent>();
                    PointLightComponent& pointLight = entity.GetComponent<PointLightComponent>();
                    uniformBufferElement.pointLights[i].positionSize = vec4(transform.position, transform.scale.x);
                    uniformBufferElement.pointLights[i].colorIntensity = pointLight.colorIntensity;
                }
                uniformBufferElement.numLights = i;
                uniformBufferElement.projectionView = frameInfo.camera->projection * frameInfo.camera->view;
                uniformBufferElement.inverseView = frameInfo.camera->inverseView;
                globalUniformBuffers[frameInfo.frameIndex]->writeAtIndex(&uniformBufferElement, 0);
                globalUniformBuffers[frameInfo.frameIndex]->flushAtIndex(0);
                // start rendering
                renderSystem->render(frameInfo);
                renderSystem->endFrame();
            }
        }
        vkDeviceWaitIdle(device.device);
    }
}