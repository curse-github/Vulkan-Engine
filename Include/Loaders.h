#ifndef ENG_LOADER
#define ENG_LOADER

#include "Helpers.h"
#include "Mesh.h"
#include "Texture.h"
#include "UboStructs.h"

namespace Eng {
    class Engine;
    namespace Loaders {
        class MeshLoader {
            static void calcNormalNtangents(const unsigned int& vertexIdx0, const unsigned int& vertexIdx1, const unsigned int& vertexIdx2, Mesh::MeshData& data);
            static unsigned int pushVertex(Mesh::Vertex&& _vertex, Mesh::MeshData& data);
            static void pushTri(Mesh::Vertex&& _vertex0, Mesh::Vertex&& _vertex1, Mesh::Vertex&& _vertex2, Mesh::MeshData& data);
            static void processLine(const std::string& line, Mesh::MeshData& data);
        public:
            static Mesh* fromObj(Device* device, const std::string& filePath);
        };
        class TextureLoader {
        public:
            static Texture* fromBmp(Device* device, const std::string& filePath);
        };
        class MaterialLoader {
            static void processLine(const std::string& filePath, const std::string& line, Mesh::MeshData& data, Engine* engine);
        public:
            static void fromMtl(Device* device, const std::string& filePath, Engine* engine);
        };
    }
}

#endif// ENG_LAODER