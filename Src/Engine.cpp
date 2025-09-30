#include "Engine.h"

namespace Eng {
    GlobalUboData uniformBufferElement;

    Engine::Engine(const std::string& windowName, const ivec2& windowSize)
        : window(windowName, windowSize), device(&window)
    {
        maxTextures = std::min(256u, device.properties.limits.maxDescriptorSetSampledImages);
        resourceManager = new ResourceManager(&device, &entitySystem, maxTextures, device.properties.limits.minUniformBufferOffsetAlignment);
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
        std::unordered_map<std::string, OwnedPointer<RendererAbstract>> renderers {};
        renderers["MeshRenderer"      ] = new MeshRenderer               (&device, globalUniforms->setLayout->descriptorSetLayout, resourceManager);
        renderers["PointLightRenderer"] = new PointLightRenderer         (&device, globalUniforms->setLayout->descriptorSetLayout, resourceManager);
        renderers["GeometryPass"      ] = new DeferredGeometryPass       (&device, globalUniforms->setLayout->descriptorSetLayout, resourceManager);
        renderers["RenderPass"        ] = new DeferredRenderPass         (&device, globalUniforms->setLayout->descriptorSetLayout, resourceManager);
        renderers["ProxyXYZ"          ] = new OnTilePostProcessRenderer  (&device, globalUniforms->setLayout->descriptorSetLayout, resourceManager, "shaders/proxy/ProxyXYZ.frag");
        renderers["ProxyWX"           ] = new OnTilePostProcessRenderer  (&device, globalUniforms->setLayout->descriptorSetLayout, resourceManager, "shaders/proxy/ProxyWX.frag");
        renderers["ProxyYZW"          ] = new OnTilePostProcessRenderer  (&device, globalUniforms->setLayout->descriptorSetLayout, resourceManager, "shaders/proxy/ProxyYZW.frag");
        renderers["ProxyXXX"          ] = new OnTilePostProcessRenderer  (&device, globalUniforms->setLayout->descriptorSetLayout, resourceManager, "shaders/proxy/ProxyXXX.frag");
        size_t renderSystemConfigIndex = 0;

        std::vector<Eng::RenderSystem::FormatOverride> deferredFormats{
            {1, VK_FORMAT_R16G16B16A16_SFLOAT},// world position, U
            {2, VK_FORMAT_R16G16B16A16_SFLOAT},// V, normal
            {3, VK_FORMAT_R16G16B16A16_SFLOAT},// tangent
            {4, VK_FORMAT_R16_SFLOAT}// material index
        };
        std::vector<RenderSystem::Config> renderSystemConfigs{
            {{{// forward rendering
                {{}, {}, {0}, 1, {renderers["MeshRenderer"], renderers["PointLightRenderer"]}}
            }}, { true }, {}},
            {{{// deferred rendering, rendering DBP
                {{}, {}, {1, 2, 3, 4}, 5, {renderers["GeometryPass"]}},
                {{}, {1, 2, 3, 4}, {0}, Swapchain::SubPassConfig::NO_DEPTH_ATTACHMENT, {renderers["RenderPass"]}},
                {{}, {}, {0}, 5, {renderers["PointLightRenderer"]}}
            }}, { true }, deferredFormats},
            {{{// deferred rendering, rendering position
                {{}, {}, {1, 2, 3, 4}, 5, {renderers["GeometryPass"]}},
                {{}, {1}, {0}, Swapchain::SubPassConfig::NO_DEPTH_ATTACHMENT, {renderers["ProxyXYZ"]}}
            }}, { true }, deferredFormats},
            {{{// deferred rendering, rendering uv
                {{}, {}, {1, 2, 3, 4}, 5, {renderers["GeometryPass"]}},
                {{}, {1, 2}, {0}, Swapchain::SubPassConfig::NO_DEPTH_ATTACHMENT, {renderers["ProxyWX"]}}
            }}, { true }, deferredFormats},
            {{{// deferred rendering, rendering normal
                {{}, {}, {1, 2, 3, 4}, 5, {renderers["GeometryPass"]}},
                {{}, {2}, {0}, Swapchain::SubPassConfig::NO_DEPTH_ATTACHMENT, {renderers["ProxyYZW"]}}
            }}, { true }, deferredFormats},
            {{{// deferred rendering, rendering tangent
                {{}, {}, {1, 2, 3, 4}, 5, {renderers["GeometryPass"]}},
                {{}, {3}, {0}, Swapchain::SubPassConfig::NO_DEPTH_ATTACHMENT, {renderers["ProxyXYZ"]}}
            }}, { true }, deferredFormats},
            {{{// deferred rendering, rendering material
                {{}, {}, {1, 2, 3, 4}, 5, {renderers["GeometryPass"]}},
                {{}, {4}, {0}, Swapchain::SubPassConfig::NO_DEPTH_ATTACHMENT, {renderers["ProxyXXX"]}}
            }}, { true }, deferredFormats},
            {{{// deferred rendering, rendering depth
                {{}, {}, {1, 2, 3, 4}, 5, {renderers["GeometryPass"]}},
                {{}, {5}, {0}, Swapchain::SubPassConfig::NO_DEPTH_ATTACHMENT, {renderers["ProxyXXX"]}}
            }}, { true }, deferredFormats}
        };
        OwnedPointer<RenderSystem> renderSystem = new RenderSystem(&window, &device, resourceManager, renderSystemConfigs[renderSystemConfigIndex]);
        
        Camera3D camera(renderSystem->getAspectRatio(), vec3(0.0f, -0.5f, -2.5f), vec3(0.0f));
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
                if (window.getKeyPressed(GLFW_KEY_N)) {
                    renderSystemConfigIndex = (renderSystemConfigIndex+1)%renderSystemConfigs.size();
                    renderSystem->setConfig(renderSystemConfigs[renderSystemConfigIndex]);
                }
            }
        }
        vkDeviceWaitIdle(device.device);
    }
}