#include <GLFW/glfw3.h>

#include <chrono>
#include <cstdio>

#include "render/Arena.hpp"
#include "render/Camera.hpp"

namespace {

constexpr char WINDOW_TITLE[] = "FruitCat Chaos 3D";
constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;

constexpr float ARENA_HALF_WIDTH = 8.0F;
constexpr float ARENA_HALF_HEIGHT = 5.0F;
constexpr float ARENA_HALF_DEPTH = 8.0F;

void configureLighting() {
    const float ambientLight[4] = {0.25F, 0.25F, 0.28F, 1.0F};
    const float diffuseLight[4] = {0.9F, 0.9F, 0.85F, 1.0F};
    const float specularLight[4] = {1.0F, 1.0F, 1.0F, 1.0F};
    const float lightPosition[4] = {12.0F, 18.0F, 14.0F, 1.0F};

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    // Specular highlights are added after blending, at full strength, so a
    // glass panel's glint stays crisp even though the panel itself is
    // rendered at low alpha.
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    glDisable(GL_LIGHTING);
}

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
    configureLighting();

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
        fruitcat::drawGlassArena(ARENA_HALF_WIDTH, ARENA_HALF_HEIGHT, ARENA_HALF_DEPTH);
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
