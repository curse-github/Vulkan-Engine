#include "Engine.h"

namespace Eng {
    GlobalUboData uniformBufferElement;

    Engine::Engine(const std::string& windowName, const ivec2& windowSize)
        : window(windowName, windowSize), device(&window), renderer(&window, &device)
    {
        maxTextures = std::min(256u, device.properties.limits.maxDescriptorSetSampledImages);
#if defined(_DEBUG) && (_DEBUG == 1)
        std::cout << "maxTextures: " << maxTextures << '\n';
#endif
        globalDescriptorPool = DescriptorPool::Builder(&device)
            .setMaxSets(Swapchain::MAX_FRAMES_IN_FLIGHT+3)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, Swapchain::MAX_FRAMES_IN_FLIGHT+2)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxTextures)
            .build();
        textureIdxs[""] = textureIdxs["White"] = storeTexture("Resources/Textures/color/White.bmp");
        textureIdxs["Normal"] = storeTexture("Resources/Textures/normal/Normal.bmp");
        window.hideCursor();
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
    GameObject::id_t Engine::addObject(
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
        GameObject object;
        object.mesh = meshes[mesh].value;
        object.transform.position = position;
        object.transform.scale = scale;
        object.transform.rotation = rotation;
        object.materialIdx = materialIdxs[materialFile+material];
        objects.emplace(object.id, (GameObject&&)object);
        return object.id;
    }
    GameObject::id_t Engine::addLight(const vec3& position, const float& size, const vec3& color, const float& intensity) {
        assert((uniformBufferElement.numLights <= MAX_LIGHTS) && "Tried to add too many lights");
        vec4 colorIntensity = vec4(color, intensity/3.0f);
        vec4 positionSize = vec4(position, size);
        // add light to lights map
        GameObject light;
        light.transform.position = position;
        light.transform.scale = vec3(size, 0.0f, 0.0f);
        light.light = new PointLightComponent(colorIntensity);
        lights.emplace(light.id, (GameObject&&)light);
        uniformBufferElement.numLights++;
        return light.id;
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
    std::chrono::_V2::system_clock::time_point lastPrint = std::chrono::high_resolution_clock::now();
    unsigned int frames = 0;
    void Engine::run() {
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
        // global uniform buffer descriptors
        for (int i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferDescriptor = globalUniformBuffers[i]->descriptorInfo();
            if (!DescriptorWriter(globalDescriptorSetLayout.value, globalDescriptorPool).writeBuffer(0, &bufferDescriptor).build(globalDescriptorSets[i]))
                std::cout << "building ubo descriptor set failed.\n";
        }
        // texture descriptors
        std::vector<VkDescriptorImageInfo> textureDescriptors(textures.size());
        for (size_t i = 0; i < textures.size(); i++) textureDescriptors[i] = textures[i]->descriptorInfo();
        // material descriptor
        VkDescriptorBufferInfo materialUniformBufferDescriptor = materialUniformBuffer->descriptorInfo();
        if (
            !(DescriptorWriter(materialDescriptorSetLayout.value, globalDescriptorPool)
            .writeImages(0, textureDescriptors.data(), textureDescriptors.size())
            .writeBuffer(1, &materialUniformBufferDescriptor).build(materialDescriptorSet))
        )
            std::cout << "building material descriptor set failed.\n";
        
        // setup rendering
        DiffuseBlinnPhongRenderSystem renderSystems(&device, renderer.getRenderPass(), globalDescriptorSetLayout->descriptorSetLayout, materialDescriptorSetLayout->descriptorSetLayout, textures.size(), materials.size(), globalDescriptorPool);
        PointLightRenderSystem pointLightRenderSystem(&device, renderer.getRenderPass(), globalDescriptorSetLayout->descriptorSetLayout);
        Camera camera;
        TransformComponent viewerTransform;
        viewerTransform.position.z = -2.5f;
        camera.setViewYXZ(viewerTransform.position, viewerTransform.rotation);
        FrameInfo frameInfo{ 0, 0.0f, 0.0f, VK_NULL_HANDLE, &camera, VK_NULL_HANDLE, materialDescriptorSet, &objects, &lights };
        std::chrono::_V2::system_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
        while(!window.shouldClose()) {
            glfwPollEvents();
            std::chrono::_V2::system_clock::time_point newTime = std::chrono::high_resolution_clock::now();
            float dt = std::chrono::duration<float, std::chrono::seconds::period>(newTime-currentTime).count();
            currentTime = newTime;
            // calc fps
            frames++;
            float diff = std::chrono::duration<float, std::chrono::seconds::period>(currentTime-lastPrint).count();
            if (diff >= 2.0f) {
                std::cout << "fps: " << (frames/diff) << '\n';
                frames=0; lastPrint = currentTime;
            }

            camera.setProj(glm::radians(50.0f), renderer.getAspectRatio(), 0.1f, 100.0f);// must be done since aspect ratio can change.
            frameInfo.commandBuffer = renderer.beginFrame();
            if (frameInfo.commandBuffer != VK_NULL_HANDLE) {
                // set frame specific info
                frameInfo.frameIndex = renderer.getFrame();
                frameInfo.t += dt;
                frameInfo.dt = dt;
                frameInfo.globalDescriptorSet = globalDescriptorSets[frameInfo.frameIndex];
                frameInfo.materialDescriptorSet = materialDescriptorSet;
                if (pollMovement(glm::min(dt, 1.0f/30.0f), viewerTransform))
                    camera.setViewYXZ(viewerTransform.position, viewerTransform.rotation);
                // let user update things
                if (updateCallback != nullptr) updateCallback(frameInfo);
                // update uniform
                unsigned int i = 0;
                for (std::pair<const GameObject::id_t, GameObject>& kv : lights) {
                    GameObject& light = kv.second;
                    uniformBufferElement.pointLights[i].positionSize = vec4(light.transform.position, light.transform.scale.x);
                    uniformBufferElement.pointLights[i].colorIntensity = light.light->colorIntensity;
                    i++;
                }
                uniformBufferElement.numLights = i;
                uniformBufferElement.projectionView = frameInfo.camera->projection * frameInfo.camera->view;
                uniformBufferElement.inverseView = frameInfo.camera->inverseView;
                globalUniformBuffers[frameInfo.frameIndex]->writeAtIndex(&uniformBufferElement, 0);
                globalUniformBuffers[frameInfo.frameIndex]->flushAtIndex(0);
                // start rendering
                renderer.beginRenderPass(frameInfo.commandBuffer);

                // order!
                renderSystems.recordObjects(frameInfo);
                pointLightRenderSystem.recordObjects(frameInfo);

                renderer.endRenderPass(frameInfo.commandBuffer);
                renderer.endFrame();
            }
        }
        vkDeviceWaitIdle(device.device);
    }
}