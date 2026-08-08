#include <windows.h>
#include <gl/GL.h>

#include <chrono>

#include "render/Camera.hpp"
#include "render/DebugCube.hpp"

namespace {

constexpr wchar_t WINDOW_CLASS_NAME[] = L"FruitCatChaosWindow";
constexpr wchar_t WINDOW_TITLE[] = L"FruitCat Chaos 3D";
constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;

LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            DestroyWindow(window);
            return 0;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(window);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        break;
    }

    return DefWindowProcW(window, message, wParam, lParam);
}

bool configurePixelFormat(HDC deviceContext) {
    PIXELFORMATDESCRIPTOR descriptor{};
    descriptor.nSize = sizeof(descriptor);
    descriptor.nVersion = 1;
    descriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    descriptor.iPixelType = PFD_TYPE_RGBA;
    descriptor.cColorBits = 32;
    descriptor.cDepthBits = 24;
    descriptor.iLayerType = PFD_MAIN_PLANE;

    const int pixelFormat = ChoosePixelFormat(deviceContext, &descriptor);
    return pixelFormat != 0 && SetPixelFormat(deviceContext, pixelFormat, &descriptor) == TRUE;
}

} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand) {
    WNDCLASSW windowClass{};
    windowClass.style = CS_OWNDC;
    windowClass.lpfnWndProc = windowProcedure;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    windowClass.lpszClassName = WINDOW_CLASS_NAME;

    if (RegisterClassW(&windowClass) == 0) {
        return 1;
    }

    RECT requestedClientArea{0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
    AdjustWindowRect(&requestedClientArea, WS_OVERLAPPEDWINDOW, FALSE);

    HWND window = CreateWindowExW(
        0,
        WINDOW_CLASS_NAME,
        WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        requestedClientArea.right - requestedClientArea.left,
        requestedClientArea.bottom - requestedClientArea.top,
        nullptr,
        nullptr,
        instance,
        nullptr
    );

    if (window == nullptr) {
        return 1;
    }

    HDC deviceContext = GetDC(window);
    if (!configurePixelFormat(deviceContext)) {
        ReleaseDC(window, deviceContext);
        return 1;
    }

    HGLRC openGlContext = wglCreateContext(deviceContext);
    if (openGlContext == nullptr || wglMakeCurrent(deviceContext, openGlContext) == FALSE) {
        if (openGlContext != nullptr) {
            wglDeleteContext(openGlContext);
        }
        ReleaseDC(window, deviceContext);
        return 1;
    }

    ShowWindow(window, showCommand);
    UpdateWindow(window);

    glEnable(GL_DEPTH_TEST);

    fruitcat::Camera camera;
    auto previousFrameTime = std::chrono::steady_clock::now();

    bool isRunning = true;
    while (isRunning) {
        const auto currentFrameTime = std::chrono::steady_clock::now();
        const std::chrono::duration<float> elapsedTime = currentFrameTime - previousFrameTime;
        previousFrameTime = currentFrameTime;

        MSG message{};
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                isRunning = false;
                break;
            }

            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        RECT clientArea{};
        GetClientRect(window, &clientArea);
        camera.update(elapsedTime.count());
        camera.apply(clientArea.right - clientArea.left, clientArea.bottom - clientArea.top);

        glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        fruitcat::drawDebugCube(2.0F);
        SwapBuffers(deviceContext);
    }

    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(openGlContext);
    ReleaseDC(window, deviceContext);
    UnregisterClassW(WINDOW_CLASS_NAME, instance);

    return 0;
}
