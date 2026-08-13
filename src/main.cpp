#include <GLFW/glfw3.h>

#include <chrono>
#include <cstdio>
#include <cmath>

#include "render/Arena.hpp"
#include "render/Camera.hpp"
#include "render/DebugSphere.hpp"
#include "render/Terrain.hpp"

namespace {

constexpr char WINDOW_TITLE[] = "FruitCat Chaos 3D";
constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;

constexpr float ARENA_HALF_WIDTH = 8.0F;
constexpr float ARENA_HALF_HEIGHT = 5.0F;
constexpr float ARENA_HALF_DEPTH = 8.0F;
constexpr float FLOOR_HEIGHT = -ARENA_HALF_HEIGHT + 0.02F;

void configureLighting() {
    const float ambientLight[4] = {0.24F, 0.32F, 0.42F, 1.0F};
    const float diffuseLight[4] = {1.0F, 0.78F, 0.46F, 1.0F};
    const float specularLight[4] = {1.0F, 1.0F, 1.0F, 1.0F};
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    // Sphere normals are authored explicitly. Single-sided lighting prevents
    // OpenGL from flipping them on the strip's reverse winding and making a
    // top-mounted light look as if it came from below.
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.78F);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.012F);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0F);
}

void updatePointLightInWorld() {
    // A real point source outside the top-left-front corner of the box.
    // It shines toward the sphere from above, so the lit hemisphere and its
    // +X/-Z projected shadow now share one unambiguous source position.
    const float lightPosition[4] = {-10.0F, 10.0F, 8.0F, 1.0F};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
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
        updatePointLightInWorld();

        glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Match the arena's full floor dimensions; the tiny height offset only
        // prevents depth conflicts with the transparent glass bottom panel.
        fruitcat::drawTerrain(ARENA_HALF_WIDTH, ARENA_HALF_DEPTH, FLOOR_HEIGHT);
        const std::chrono::duration<float> totalTime = currentFrameTime.time_since_epoch();
        fruitcat::drawDebugSphere(totalTime.count(), FLOOR_HEIGHT);
        fruitcat::drawGlassArena(ARENA_HALF_WIDTH, ARENA_HALF_HEIGHT, ARENA_HALF_DEPTH);
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
