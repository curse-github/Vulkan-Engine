#ifndef ENG_WINDOW
#define ENG_WINDOW

#include "Helpers.h"

namespace Eng {
    class Window {
        GLFWwindow* window;
        std::unordered_map<int, int> keys{};

        static void frameBufferResizedCallback(GLFWwindow* glfwWindow, int width, int height);
        static void cursorPositionChangedCallback(GLFWwindow* glfwWindow, double xpos, double ypos);
        int getKey(const int& key);
    public:
        std::string name;
        ivec2 size;
        bool frameBufferResized;
        Window(const std::string& _name, const ivec2& _size);
        Window(const Window& copy) = delete;
        Window& operator=(const Window& copy) = delete;
        Window(Window&& move) = delete;
        Window& operator=(Window&& move) = delete;
        ~Window();

        dvec2 lastMousePosition;
        dvec2 currentMousePosition;

        bool shouldClose();
        bool createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);
        bool getKeyPressed(const int& key);
        bool getKeyHeld(const int& key);
        vec2 getMouseChange();
        void hideCursor();
        void showCursor();
    };
}

#endif