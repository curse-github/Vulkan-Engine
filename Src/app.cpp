#include "app.h"
using namespace Eng;

std::vector<ECS_id_t> lightIds{};
const vec3 base(0.0f, -0.5f, 0.25f);
const vec3 mult(1.125f, 0.1333f, 1.125f);
const float speedXZ = 1.25f;// revolutions per second

ECS_id_t floorId;
const std::vector<std::string> floorMaterials{
    "Tiles",
    "Metal",
    "Rubber",
    "Rock",
    "DebugUV"
};
unsigned int floorMaterialIndex = 0;

int main(int argc, char** argv) {
    Engine engine("Window!", {1920, 1080});
    engine.loadMtlFile("Resources/Materials/Materials.mtl");
    engine.loadMtlFile("Resources/Materials/StanfordBunny.mtl");
    engine.addMeshRendereredEntity(// monkey1
        {-1.75f, -0.5f, 2.25f},// position
        {1.0f, 0.75f, 0.75f},// scale
        {0.0f, -DEG45, 0.0f},// rotation
    "Resources/Models/suzanne.obj", "Moss");
    engine.addMeshRendereredEntity(// monkey2
        {0.0f, -0.5f, 2.75f},// position
        {0.8f, 0.8f,  0.8f},// scale
        {0.0f, DEG180, 0.0f},// rotation
    "Resources/Models/suzanne_random_island_trick.obj", "SuzanneIslandTrick");
    engine.addMeshRendereredEntity(// bunny
        {1.75f, 0.5f, 2.25f},// position
        {0.85f, 0.85f, 0.85f},// scale
        {DEG180, 0.0f, 0.0f},// rotation
    "Resources/Models/StanfordBunny.obj", "StanfordBunny");

    floorId = engine.addMeshRendereredEntity(// floor
        {0.0f, 0.5f, 1.5f},// position
        {6.0f, 6.0f, 6.0f},// scale
        {0.0f, 0.0f, 0.0f},// rotation
    "Resources/Models/Quad.obj", floorMaterials[floorMaterialIndex]);
    
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
    if (frameInfo.getKeyPressed(GLFW_KEY_UP)) {
        floorMaterialIndex++;
        changed = true;
    }
    if (frameInfo.getKeyPressed(GLFW_KEY_DOWN)) {
        floorMaterialIndex--;
        changed = true;
    }
    if (changed) {
        floorMaterialIndex = (floorMaterialIndex+floorMaterials.size())%floorMaterials.size();
        Entity& floor = frameInfo.entitySystem->GetEntity(floorId);
        floor.GetComponent<MeshRendererComponent>().material = floorMaterials[floorMaterialIndex];
    }
}