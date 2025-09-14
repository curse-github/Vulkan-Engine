#include "app.h"
const vec3 base(0.0f, -0.5f, 0.25f);
const vec3 mult(1.125f, 0.1333f, 1.125f);
const float speedXZ = 1.25f;// revolutions per second
int main(int argc, char** argv) {
    Eng::Engine engine("Window!", {1920, 1080});
    engine.addObject(// monkey1
        {-1.75f, -0.5f, 2.25f},// position
        {1.0f, 0.75f, 0.75f},// scale
        {0.0f, -DEG45, 0.0f},// rotation
    "Resources/Models/suzanne.obj", "Resources/Materials/materials.mtl", "Moss", 0.25f);
    engine.addObject(// monkey2
        {0.0f, -0.5f, 2.75f},// position
        {0.8f, 0.8f,  0.8f},// scale
        {0.0f, DEG180, 0.0f},// rotation
    "Resources/Models/suzanne_random_island_trick.obj", "Resources/Materials/materials.mtl", "SuzanneIslandTrick", 1.0f);
    engine.addObject(// monkey3
        {1.75f, -0.5f, 2.25f},// position
        {1.0f, 0.75f, 0.75f},// scale
        {0.0f, DEG45, 0.0f},// rotation
    "Resources/Models/suzanne.obj", "Resources/Materials/materials.mtl", "Rubber", 1.0f);
    engine.addObject(// floor
        {0.0f, 0.5f, 1.5f},// position
        {6.0f, 6.0f, 6.0f},// scale
        {0.0f, 0.0f, 0.0f},// rotation
    "Resources/Models/Quad.obj", "Resources/Materials/materials.mtl", "Tiles", 1.0f);
    
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
        lightIds.push_back(engine.addLight(
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
void update(Eng::FrameInfo& frameInfo) {
    unsigned int numLights = static_cast<unsigned int>(lightIds.size());
    int i = 0;
    Eng::GameObject::Map& map = *frameInfo.lights;
    for (const Eng::GameObject::id_t& id : lightIds) {
        Eng::GameObject& light = map[id];
        light.transform.position.x = base.x+mult.x*cos(DEG360/numLights*i+glm::mod(frameInfo.t*speedXZ, DEG360));
        light.transform.position.z = base.z+mult.z*sin(DEG360/numLights*i+glm::mod(frameInfo.t*speedXZ, DEG360));
        i++;
    }
}