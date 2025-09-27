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
        resourceManager = new ResourceManager(&device, globalDescriptorPool, maxTextures, device.properties.limits.minUniformBufferOffsetAlignment);
        window.hideCursor();
        entitySystem.RegisterComponent<TransformComponent>();
        entitySystem.RegisterComponent<MeshRendererComponent>();
        entitySystem.RegisterComponent<PointLightComponent>();
    }
    Engine::~Engine() {
    }

    void Engine::loadMtlFile(const std::string& mtlFile) {
        resourceManager->loadMtlFile(mtlFile);
    }
    ECS_id_t Engine::addMeshRendereredEntity(
        const vec3& position, const vec3& scale, const vec3& rotation, const std::string& mesh,
        const std::string& materialName
    ) {
        Entity* entity = entitySystem.CreateEntity();
        TransformComponent& transform = entity->AddComponent<TransformComponent>(new TransformComponent());
        transform.position = position;
        transform.scale = scale;
        transform.rotation = rotation;
        MeshRendererComponent& meshRenderer = entity->AddComponent<MeshRendererComponent>(new MeshRendererComponent());
        meshRenderer.mesh = mesh;
        meshRenderer.material = materialName;
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
    void Engine::run() {
        // create uniform buffers
        ResourceManager::MappedUniformData* globalUniforms = resourceManager->getMappedUniform(Swapchain::MAX_FRAMES_IN_FLIGHT, sizeof(GlobalUboData), 1u);
        resourceManager->createMaterialUniform();
        // setup rendering
        std::vector<RendererAbstract*> renderers {
            new DiffuseBlinnPhongRenderer (&device, globalUniforms->setLayout->descriptorSetLayout, resourceManager),
            new PointLightRenderer        (&device, globalUniforms->setLayout->descriptorSetLayout, resourceManager),
            new OnTilePostProcessRenderer (&device, globalUniforms->setLayout->descriptorSetLayout, resourceManager, "shaders/Fog.frag"),
            new OffTilePostProcessRenderer(&device, globalUniforms->setLayout->descriptorSetLayout, resourceManager, "shaders/Blur.frag")
        };
        size_t renderSystemConfigIndex = 0;
        std::vector<std::vector<std::vector<RenderSystem::SubPass>>> renderSystemConfigs{
            {{// no effects
                {{}, {}, {0}, 1, {renderers[0], renderers[1]}}
            }},
            {{// with fog
                {{}, {}, {1}, 2, {renderers[0], renderers[1]}},
                {{}, {1, 2}, {0}, Swapchain::SubPassConfig::NO_DEPTH_ATTACHMENT, {renderers[2]}}
            }},
            {{// with blur
                {{}, {}, {1}, 2, {renderers[0], renderers[1]}}
            },{
                {{1}, {}, {0}, Swapchain::SubPassConfig::NO_DEPTH_ATTACHMENT, {renderers[3]}}
            }}
        };
        OwnedPointer<RenderSystem> renderSystem = new RenderSystem(&window, &device, renderSystemConfigs[renderSystemConfigIndex], globalDescriptorPool);
        
        Camera3D camera(renderSystem->getAspectRatio(), vec3(0.0f, 0.0f, -2.5f), vec3(0.0f));
        FrameInfo frameInfo(&window, &camera, &entitySystem);
        while(!window.shouldClose()) {
            glfwPollEvents();
            frameInfo.commandBuffer = renderSystem->beginFrame();
            if (frameInfo.commandBuffer != VK_NULL_HANDLE) {
                // set frame specific info
                frameInfo.imageIndex = renderSystem->getImage();
                frameInfo.frameIndex = renderSystem->getFrame();
                frameInfo.globalDescriptorSet = globalUniforms->sets[frameInfo.frameIndex];
                frameInfo.updateTime();
                camera.pollMovement(frameInfo, keys);
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
                globalUniforms->buffers[frameInfo.frameIndex]->writeAtIndex(&uniformBufferElement, 0);
                globalUniforms->buffers[frameInfo.frameIndex]->flushAtIndex(0);
                // start rendering
                renderSystem->render(frameInfo);
                renderSystem->endFrame();
                bool changed = false;
                if (window.getKeyPressed(GLFW_KEY_RIGHT)) {
                    renderSystemConfigIndex++;
                    changed = true;
                }
                if (window.getKeyPressed(GLFW_KEY_LEFT)) {
                    renderSystemConfigIndex--;
                    changed = true;
                }
                if (changed) {
                    renderSystemConfigIndex = ((renderSystemConfigIndex+renderSystemConfigs.size())%renderSystemConfigs.size());
                    renderSystem->setConfig(renderSystemConfigs[renderSystemConfigIndex]);
                }
            }
        }
        vkDeviceWaitIdle(device.device);
        for (size_t i = 0; i < renderers.size(); i++)
            delete renderers[i];
        renderers.clear();
    }
}