cmake_minimum_required(VERSION 3.20)

project(FruitCatChaos LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(OpenMP REQUIRED)

add_executable(fruitcat-chaos
    src/main.cpp
    src/core/OpenMpRuntime.cpp
    src/core/ProgramOptions.cpp
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
target_link_libraries(fruitcat-chaos PRIVATE glfw OpenGL::GL OpenMP::OpenMP_CXX)

include(CTest)
if(BUILD_TESTING)
    add_executable(simulation-seed-test
        tests/SimulationSeedTest.cpp
        src/simulation/Simulation.cpp
    )
    target_include_directories(simulation-seed-test PRIVATE include)
    if(MSVC)
        target_compile_options(simulation-seed-test PRIVATE /W4)
    else()
        target_compile_options(simulation-seed-test PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    if(MINGW)
        target_link_options(simulation-seed-test PRIVATE -static)
    endif()
    add_test(NAME simulation-seed-test COMMAND simulation-seed-test)

    add_executable(program-options-test
        tests/ProgramOptionsTest.cpp
        src/core/ProgramOptions.cpp
    )
    target_include_directories(program-options-test PRIVATE include)
    if(MSVC)
        target_compile_options(program-options-test PRIVATE /W4)
    else()
        target_compile_options(program-options-test PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    if(MINGW)
        target_link_options(program-options-test PRIVATE -static)
    endif()
    add_test(NAME program-options-test COMMAND program-options-test)

    add_executable(openmp-runtime-test
        tests/OpenMpRuntimeTest.cpp
        src/core/OpenMpRuntime.cpp
    )
    target_include_directories(openmp-runtime-test PRIVATE include)
    target_link_libraries(openmp-runtime-test PRIVATE OpenMP::OpenMP_CXX)
    if(MSVC)
        target_compile_options(openmp-runtime-test PRIVATE /W4)
    else()
        target_compile_options(openmp-runtime-test PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    if(MINGW)
        target_link_options(openmp-runtime-test PRIVATE -static)
        get_filename_component(MINGW_COMPILER_DIRECTORY "${CMAKE_CXX_COMPILER}" DIRECTORY)
        add_custom_command(TARGET openmp-runtime-test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${MINGW_COMPILER_DIRECTORY}/libgomp-1.dll"
                "$<TARGET_FILE_DIR:openmp-runtime-test>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${MINGW_COMPILER_DIRECTORY}/libwinpthread-1.dll"
                "$<TARGET_FILE_DIR:openmp-runtime-test>"
        )
    endif()
    add_test(NAME openmp-runtime-test COMMAND openmp-runtime-test)
endif()
