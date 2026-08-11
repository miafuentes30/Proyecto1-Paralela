#include <GLFW/glfw3.h>

#include <chrono>
#include <cstdio>

#include "render/Camera.hpp"
#include "render/DebugCube.hpp"

namespace {

constexpr char WINDOW_TITLE[] = "FruitCat Chaos 3D";
constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;

void keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void errorCallback(int error, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

} // namespace

int main() {
    glfwSetErrorCallback(errorCallback);

    if (!glfwInit()) {
        return 1;
    }

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, nullptr, nullptr);
    if (window == nullptr) {
        glfwTerminate();
        return 1;
    }

    glfwSetKeyCallback(window, keyCallback);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glEnable(GL_DEPTH_TEST);

    fruitcat::Camera camera;
    auto previousFrameTime = std::chrono::steady_clock::now();

    while (!glfwWindowShouldClose(window)) {
        const auto currentFrameTime = std::chrono::steady_clock::now();
        const std::chrono::duration<float> elapsedTime = currentFrameTime - previousFrameTime;
        previousFrameTime = currentFrameTime;

        glfwPollEvents();

        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

        camera.update(elapsedTime.count());
        camera.apply(framebufferWidth, framebufferHeight);

        glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        fruitcat::drawDebugCube(2.0F);
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
