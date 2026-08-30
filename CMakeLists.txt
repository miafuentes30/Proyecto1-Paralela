cmake_minimum_required(VERSION 3.20)

project(FruitCatChaos LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(fruitcat-chaos
    src/main.cpp
    src/render/Arena.cpp
    src/render/Backdrop.cpp
    src/render/Camera.cpp
    src/render/FruitCatRenderer.cpp
    src/render/SpriteLibrary.cpp
    src/render/Terrain.cpp
    src/render/TextOverlay.cpp
    src/simulation/Simulation.cpp
)

target_include_directories(fruitcat-chaos PRIVATE include third_party)
target_link_libraries(fruitcat-chaos PRIVATE glfw OpenGL::GL)
