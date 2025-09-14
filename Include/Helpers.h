#ifndef ENG_HELPERS
#define ENG_HELPERS
// #define VSYNC

#define DEG45 0.78539816339f
#define DEG90 1.5707963268f
#define DEG180 3.14159265359f
#define DEG270 4.71238898039f
#define DEG360 6.28318530718f

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <GLFW/glfw3.h>
// #include <vulkan/vk_enum_string_helper.h>// string_VkResult(res)
#include <glm/vec2.hpp>
using glm::ivec2;
using glm::vec2;
using glm::dvec2;
#include <glm/vec3.hpp>
using glm::vec3;
#include <glm/vec4.hpp>
using glm::vec4;
#include <glm/mat3x3.hpp>
using glm::mat3;
#include <glm/mat4x4.hpp>
using glm::mat4;
#include <glm/gtx/transform.hpp>
#include <glm/gtx/hash.hpp>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <string>
#include <set>
#include <unordered_set>
#include <cstring>
#include <array>
#include <cassert>
#include <chrono>
#include <unordered_map>
#include <memory>
#include <map>
#include <algorithm>
#include "Camera.h"

namespace Eng {
    template<typename T>
    struct OwnedPointer {
        T* value;
        OwnedPointer(T* _value = nullptr) : value(_value) {};
        OwnedPointer(const OwnedPointer<T>& copy) = delete;
        OwnedPointer& operator=(const OwnedPointer<T>& copy) = delete;
        OwnedPointer(OwnedPointer<T>&& move) : value(move.value) {
            move.value = nullptr;
        };
        OwnedPointer& operator=(OwnedPointer<T>&& move) {
            value = move.value;
            move.value = nullptr;
            return *this;
        }
        ~OwnedPointer() { if (value != nullptr) delete value; };
        OwnedPointer& operator=(T* _value) {
            if (value != nullptr) delete value;
            value = _value;
            return *this;
        };
        T* operator->() { return value; }
        const T* operator->() const { return value; }
        operator T*() { return value; }
        operator const T*() const { return value; }
        operator T&() { return *value; }
        operator const T&() const { return *value; }
    };
    struct KeyMappings {
        int moveLeft = GLFW_KEY_A;
        int moveRight = GLFW_KEY_D;
        int moveForward = GLFW_KEY_W;
        int moveBackward = GLFW_KEY_S;
        int moveUp = GLFW_KEY_SPACE;
        int moveDown = GLFW_KEY_LEFT_SHIFT;
        int pause = GLFW_KEY_ESCAPE;
    };
    struct KeyStates {
        int moveLeft = GLFW_RELEASE;
        int moveRight = GLFW_RELEASE;
        int moveForward = GLFW_RELEASE;
        int moveBackward = GLFW_RELEASE;
        int moveUp = GLFW_RELEASE;
        int moveDown = GLFW_RELEASE;
        int pause = GLFW_RELEASE;
    };
}

std::vector<char> readFile(const std::string& path);
std::vector<unsigned char> readFileBytes(const std::string& path);
// from: https://stackoverflow.com/a/57595105
template <typename T, typename... Rest>
void hashCombine(std::size_t& seed, const T& v, const Rest&... rest) {
    seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    (hashCombine(seed, rest), ...);
};

#endif