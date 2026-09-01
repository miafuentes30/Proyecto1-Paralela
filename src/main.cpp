#include <GLFW/glfw3.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>

#include "core/ProgramOptions.hpp"
#include "render/Arena.hpp"
#include "render/Backdrop.hpp"
#include "render/Camera.hpp"
#include "render/FruitCatRenderer.hpp"
#include "render/Terrain.hpp"
#include "render/TextOverlay.hpp"
#include "simulation/Simulation.hpp"

namespace {

constexpr char WINDOW_TITLE[] = "FruitCat Chaos 3D";
constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;

constexpr float ARENA_HALF_WIDTH = 8.0F;
constexpr float ARENA_HALF_HEIGHT = 7.0F;
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

void toggleFullscreen(GLFWwindow* window);

void keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    if (action != GLFW_PRESS) {
        return;
    }
    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    } else if (key == GLFW_KEY_F11 || key == GLFW_KEY_F) {
        toggleFullscreen(window);
    }
}

void errorCallback(int error, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

/*
 * VERSION ANTERIOR:
 * El analizador estaba acoplado a main.cpp y solo aceptaba N y pantalla
 * completa. Se reemplazo por ProgramOptions para validar tambien modo, hilos
 * y semilla sin inicializar GLFW, lo que permite probarlo de forma aislada.
 *
 * Codigo anterior:
 * struct Options {
 *     int catCount = DEFAULT_CAT_COUNT;
 *     bool fullscreen = false;
 * };
 *
 * bool parseArguments(int argc, char** argv, Options& options) {
 *     bool sawCatCount = false;
 *     for (int index = 1; index < argc; ++index) {
 *         const std::string argument = argv[index];
 *         if (argument == "-f" || argument == "--fullscreen") {
 *             options.fullscreen = true;
 *             continue;
 *         }
 *         if (sawCatCount) {
 *             return false;
 *         }
 *
 *         char* end = nullptr;
 *         const long parsed = std::strtol(argument.c_str(), &end, 10);
 *         if (argument.empty() || *end != '\0' || parsed < 1 || parsed > MAX_CAT_COUNT) {
 *             return false;
 *         }
 *         options.catCount = static_cast<int>(parsed);
 *         sawCatCount = true;
 *     }
 *     return true;
 * }
 */

struct WindowState {
    bool fullscreen = false;
    int windowedWidth = WINDOW_WIDTH;
    int windowedHeight = WINDOW_HEIGHT;
};

void toggleFullscreen(GLFWwindow* window) {
    auto* state = static_cast<WindowState*>(glfwGetWindowUserPointer(window));
    if (state == nullptr) {
        return;
    }

    if (state->fullscreen) {
        // GLFW wants a position here, but Wayland ignores it and cannot report
        // the real one, so a fixed corner avoids a spurious error there without
        // changing anything on X11 or Windows.
        glfwSetWindowMonitor(window, nullptr, 100, 100,
                             state->windowedWidth, state->windowedHeight, GLFW_DONT_CARE);
        state->fullscreen = false;
        return;
    }

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = monitor != nullptr ? glfwGetVideoMode(monitor) : nullptr;
    if (mode == nullptr) {
        return;
    }
    glfwGetWindowSize(window, &state->windowedWidth, &state->windowedHeight);
    glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    state->fullscreen = true;
}

void drawHud(int framebufferWidth, int framebufferHeight, int totalCats, int activeCats, float fps) {
    char line[96];
    std::snprintf(line, sizeof(line), "SEQUENTIAL | N %d | ACTIVE %d | FPS %.1f", totalCats, activeCats, fps);
    // Scale with the framebuffer so the HUD reads the same on a high-DPI or
    // fractionally scaled display, but keep it a whole number of pixels so the
    // bitmap font stays aligned to the pixel grid and looks crisp.
    const float pixelSize = std::max(2.0F, std::floor(static_cast<float>(framebufferHeight) / 300.0F));
    fruitcat::drawScreenText(framebufferWidth, framebufferHeight, pixelSize * 4.0F, pixelSize * 4.0F, pixelSize, line);
}

void updateWindowTitle(GLFWwindow* window, int totalCats, int activeCats, float fps) {
    const std::string title = "FruitCat Chaos 3D | Sequential | N: " + std::to_string(totalCats)
        + " | Active: " + std::to_string(activeCats) + " | FPS: " + std::to_string(static_cast<int>(fps + 0.5F));
    glfwSetWindowTitle(window, title.c_str());
}

} // namespace

int main(int argc, char** argv) {
    const fruitcat::OptionsParseResult parseResult = fruitcat::parseProgramOptions(argc, argv);
    const std::string usage = fruitcat::programUsage(argc > 0 ? argv[0] : "fruitcat-chaos");
    if (parseResult.status == fruitcat::OptionsParseStatus::Help) {
        std::fputs(usage.c_str(), stdout);
        return 0;
    }
    if (parseResult.status == fruitcat::OptionsParseStatus::Error) {
        std::fprintf(stderr, "Error: %s\n%s", parseResult.errorMessage.c_str(), usage.c_str());
        return 1;
    }
    const fruitcat::ProgramOptions& options = parseResult.options;

    if (options.mode == fruitcat::ExecutionMode::Parallel) {
        std::puts("El modo parallel fue solicitado correctamente, pero se habilitara en la siguiente fase al configurar OpenMP.");
        return 0;
    }

    std::printf("Configuracion: modo=%s, N=%d, hilos=%d, semilla=%u\n",
                fruitcat::executionModeName(options.mode), options.catCount,
                options.threadCount, static_cast<unsigned int>(options.seed));

    glfwSetErrorCallback(errorCallback);

    if (!glfwInit()) {
        return 1;
    }

    GLFWmonitor* startMonitor = nullptr;
    int createWidth = WINDOW_WIDTH;
    int createHeight = WINDOW_HEIGHT;
    if (options.fullscreen) {
        startMonitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = startMonitor != nullptr ? glfwGetVideoMode(startMonitor) : nullptr;
        if (mode != nullptr) {
            createWidth = mode->width;
            createHeight = mode->height;
        } else {
            startMonitor = nullptr;
        }
    }

    GLFWwindow* window = glfwCreateWindow(createWidth, createHeight, WINDOW_TITLE, startMonitor, nullptr);
    if (window == nullptr) {
        glfwTerminate();
        return 1;
    }

    WindowState windowState{startMonitor != nullptr, WINDOW_WIDTH, WINDOW_HEIGHT};
    glfwSetWindowUserPointer(window, &windowState);
    glfwSetKeyCallback(window, keyCallback);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glEnable(GL_DEPTH_TEST);
    configureLighting();

    fruitcat::Camera camera;
    camera.setSubject(ARENA_HALF_WIDTH, ARENA_HALF_HEIGHT, ARENA_HALF_DEPTH);
    const fruitcat::ArenaBounds bounds{
        -ARENA_HALF_WIDTH, ARENA_HALF_WIDTH,
        FLOOR_HEIGHT, ARENA_HALF_HEIGHT,
        -ARENA_HALF_DEPTH, ARENA_HALF_DEPTH
    };
    fruitcat::Simulation simulation(options.catCount, bounds, options.seed);
    auto previousFrameTime = std::chrono::steady_clock::now();
    float fpsAccumulator = 0.0F;
    int framesSinceTitleUpdate = 0;
    float measuredFps = 0.0F;

    while (!glfwWindowShouldClose(window)) {
        const auto currentFrameTime = std::chrono::steady_clock::now();
        const std::chrono::duration<float> elapsedTime = currentFrameTime - previousFrameTime;
        previousFrameTime = currentFrameTime;
        fpsAccumulator += elapsedTime.count();
        ++framesSinceTitleUpdate;

        glfwPollEvents();

        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

        camera.update(elapsedTime.count());
        camera.apply(framebufferWidth, framebufferHeight);
        updatePointLightInWorld();
        simulation.update(elapsedTime.count());

        glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        fruitcat::drawSkyGradient();
        fruitcat::drawStarfield();
        // Match the arena's full floor dimensions; the tiny height offset only
        // prevents depth conflicts with the transparent glass bottom panel.
        fruitcat::drawTerrain(ARENA_HALF_WIDTH, ARENA_HALF_DEPTH, FLOOR_HEIGHT);
        fruitcat::drawFruitCats(simulation.cats(), FLOOR_HEIGHT);
        fruitcat::drawGlassArena(ARENA_HALF_WIDTH, ARENA_HALF_HEIGHT, ARENA_HALF_DEPTH);
        drawHud(framebufferWidth, framebufferHeight, simulation.totalCats(), simulation.activeCats(), measuredFps);
        glfwSwapBuffers(window);

        if (fpsAccumulator >= 0.5F) {
            measuredFps = static_cast<float>(framesSinceTitleUpdate) / fpsAccumulator;
            updateWindowTitle(window, simulation.totalCats(), simulation.activeCats(), measuredFps);
            fpsAccumulator = 0.0F;
            framesSinceTitleUpdate = 0;
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
