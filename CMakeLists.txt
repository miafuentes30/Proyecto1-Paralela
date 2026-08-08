cmake_minimum_required(VERSION 3.20)

project(FruitCatChaos LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(fruitcat-chaos WIN32
    src/main.cpp
    src/render/Camera.cpp
    src/render/DebugCube.cpp
)

target_include_directories(fruitcat-chaos PRIVATE include)

if (WIN32)
    target_link_libraries(fruitcat-chaos PRIVATE opengl32)
else()
    message(FATAL_ERROR "La base inicial de ventana está configurada para Windows.")
endif()
