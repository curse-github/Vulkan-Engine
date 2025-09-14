#include "Window.h"

namespace Eng {
    Window::Window(const std::string& _name, const ivec2& _size) : name(_name), size(_size){
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        window = glfwCreateWindow(size.x, size.y, name.c_str(), nullptr, nullptr);// last argument is for fullscreen.
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, frameBufferResizedCallback);
        
        glfwSetCursorPosCallback(window, cursorPositionChangedCallback);
    }
    Window::~Window() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }



    
    void Window::frameBufferResizedCallback(GLFWwindow* glfwWindow, int width, int height) {
        Window* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
        window->frameBufferResized = true;
        window->size = {width, height};
    }
    void Window::cursorPositionChangedCallback(GLFWwindow* glfwWindow, double xpos, double ypos) {
        Window* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
        window->currentMousePosition = {xpos, ypos};
    }
    
    bool Window::shouldClose() {
        return glfwWindowShouldClose(window);
    }
    bool Window::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
        if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS)
            throw std::runtime_error("Failed to create window surface");
        return true;
    }
    int Window::getKey(const int& key) {
        int state = glfwGetKey(window, key);
        int ret = ((state == GLFW_PRESS) ? ((keys[key] == GLFW_PRESS) ? GLFW_REPEAT : GLFW_PRESS) : GLFW_RELEASE);
        keys[key] = state;
        return ret;
    }
    bool Window::getKeyPressed(const int& key) {
        return getKey(key) == GLFW_PRESS;
    }
    bool Window::getKeyHeld(const int& key) {
        return getKey(key) != GLFW_RELEASE;
    }
    vec2 Window::getMouseChange() {
        vec2 temp = {(float)(currentMousePosition.x-lastMousePosition.x)/size.y, (float)(currentMousePosition.y-lastMousePosition.y)/size.y};
        lastMousePosition = currentMousePosition;
        return temp;
    }
    void Window::hideCursor() {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        lastMousePosition = currentMousePosition;
    }
    void Window::showCursor() {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        lastMousePosition = currentMousePosition;
    }
}