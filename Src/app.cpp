#include "app.h"
using namespace Eng;

std::vector<ECS_id_t> lightIds{};
const vec3 base(0.0f, -0.5f, 0.25f);
const vec3 mult(1.125f, 0.1333f, 1.125f);
const float speedXZ = 1.25f;// revolutions per second

std::vector<ECS_id_t> objectIds{};
const std::vector<std::string> objectModelsAndMaterials{
    "Resources/Models/suzanne.obj", "White",
    "Resources/Models/suzanne_random_island_trick.obj", "SuzanneIslandTrick",
    "Resources/Models/StanfordBunny.obj", "StanfordBunny",
    "Resources/Models/suzanne_random_island_trick.obj", "Rubber",
    "Resources/Models/StanfordBunny.obj", "Metal",
    "Resources/Models/suzanne.obj", "DebugUV",
    "Resources/Models/suzanne.obj", "Moss",
    "Resources/Models/suzanne.obj", "Rubber",
    "Resources/Models/suzanne.obj", "Metal",
    "Resources/Models/suzanne.obj", "Rock",
};
int objectModelOffset = 0;

ECS_id_t floorId;
const std::vector<std::string> floorMaterials{
    "Tiles",
    "Metal",
    "Rubber",
    "Rock",
    "DebugUV"
};
int floorMaterialIndex = 0;

int main(int argc, char** argv) {
    Engine engine("Window!", {1920, 1080});
    engine.loadMtlFile("Resources/Materials/Materials.mtl");
    engine.loadMtlFile("Resources/Materials/Special.mtl");
    objectIds.push_back(engine.addMeshRendereredEntity(// monkey1
        {-1.75f, 0.0f, 2.25f},// position
        {1.0f, 1.0f, 1.0f},// scale
        {0.0f, -DEG45, 0.0f},// rotation
        objectModelsAndMaterials[(objectModelOffset+0ull)%objectModelsAndMaterials.size()],
        objectModelsAndMaterials[(objectModelOffset+1ull)%objectModelsAndMaterials.size()]
    ));
    objectIds.push_back(engine.addMeshRendereredEntity(// monkey2
        {0.0f, 0.0f, 2.75f},// position
        {1.0f, 1.0f, 1.0f},// scale
        {0.0f, 0.0f, 0.0f},// rotation
        objectModelsAndMaterials[(objectModelOffset+2ull)%objectModelsAndMaterials.size()],
        objectModelsAndMaterials[(objectModelOffset+3ull)%objectModelsAndMaterials.size()]
    ));
    objectIds.push_back(engine.addMeshRendereredEntity(// bunny
        {1.75f, 0.0f, 2.25f},// position
        {1.0f, 1.0f, 1.0f},// scale
        {0.0f, DEG45, 0.0f},// rotation
        objectModelsAndMaterials[(objectModelOffset+4ull)%objectModelsAndMaterials.size()],
        objectModelsAndMaterials[(objectModelOffset+5ull)%objectModelsAndMaterials.size()]
    ));

    floorId = engine.addMeshRendereredEntity(// floor
        {0.0f, 0.0f, 1.5f},// position
        {6.0f, 6.0f, 6.0f},// scale
        {0.0f, 0.0f, 0.0f},// rotation
        "Resources/Models/Quad.obj",
        floorMaterials[floorMaterialIndex]
    );
    
    std::vector<vec3> colors = {
        glm::normalize(vec3(1.0f, 0.0f, 0.0f)),
        glm::normalize(vec3(1.0f, 1.0f, 0.0f)),
        glm::normalize(vec3(0.0f, 1.0f, 0.0f)),
        glm::normalize(vec3(0.0f, 1.0f, 1.0f)),
        glm::normalize(vec3(0.0f, 0.0f, 1.0f)),
        glm::normalize(vec3(1.0f, 0.0f, 1.0f))
    };
    unsigned int numLights = static_cast<unsigned int>(colors.size());
    for (size_t i = 0; i < colors.size(); i++) {
        lightIds.push_back(engine.addLightEntity(
            {base.x+mult.x*cos(DEG360/numLights*i), base.y, base.z+mult.z*sin(DEG360/numLights*i)},// position
            0.0666f,// size
            colors[i],// color
            1.0f// intensity
        ));
    }

    engine.setUpdate(update);
    engine.start();
    engine.run();
    return 0;
}
void update(FrameInfo& frameInfo) {
    unsigned int numLights = static_cast<unsigned int>(lightIds.size());
    size_t i = 0;
    for (const ECS_id_t& lightId : lightIds) {
        Entity& entity = frameInfo.entitySystem->GetEntity(lightId);
        TransformComponent& transform = entity.GetComponent<TransformComponent>();
        transform.position.x = base.x+mult.x*cos(DEG360/numLights*i+glm::mod(frameInfo.t*speedXZ, DEG360));
        transform.position.z = base.z+mult.z*sin(DEG360/numLights*i+glm::mod(frameInfo.t*speedXZ, DEG360));
        i++;
    }
    bool changed = false;
    if (frameInfo.getKeyPressed(GLFW_KEY_RIGHT)) { floorMaterialIndex++; changed = true; }
    if (frameInfo.getKeyPressed(GLFW_KEY_LEFT)) { floorMaterialIndex += floorMaterials.size()-1; changed = true; }
    if (changed) {
        floorMaterialIndex = floorMaterialIndex%floorMaterials.size();
        Entity& floor = frameInfo.entitySystem->GetEntity(floorId);
        floor.GetComponent<MeshRendererComponent>().material = floorMaterials[floorMaterialIndex];
    }
    changed = false;
    if (frameInfo.getKeyPressed(GLFW_KEY_UP)) { objectModelOffset += 2; changed = true; }
    if (frameInfo.getKeyPressed(GLFW_KEY_DOWN)) { objectModelOffset += objectModelsAndMaterials.size()-2; changed = true; }
    if (changed) {
        objectModelOffset %= objectModelsAndMaterials.size();
        for (size_t i = 0; i < objectIds.size(); i++) {
            MeshRendererComponent& meshRenderer = frameInfo.entitySystem->GetEntity(objectIds[i]).GetComponent<MeshRendererComponent>();
            meshRenderer.mesh = objectModelsAndMaterials[(objectModelOffset+i*2ull)%objectModelsAndMaterials.size()];
            meshRenderer.material = objectModelsAndMaterials[(objectModelOffset+i*2ull+1ull)%objectModelsAndMaterials.size()];
        }
    }
}