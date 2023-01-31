cmake_minimum_required(VERSION "3.2")

project(mirage)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_MBCS -DGLM_ENABLE_EXPERIMENTAL -Wordered-compare-function-pointers")

file(GLOB_RECURSE MIRAGE_SOURCES "src/*.cpp" "src/*.hpp")
add_executable(mirage "${MIRAGE_SOURCES}")

include_directories("${CMAKE_SOURCE_DIR}/ext/stb")
include_directories("${CMAKE_SOURCE_DIR}/ext/glm")
include_directories("${CMAKE_SOURCE_DIR}/ext/imgui/")
include_directories("${CMAKE_SOURCE_DIR}/ext/imgui-console/include")

find_package(Vulkan)

if (Vulkan_FOUND)
    message(STATUS "Found Vulkan package in system ${Vulkan_LIBRARY}")
    include_directories("${Vulkan_INCLUDE_DIRS}")
    link_libraries("${Vulkan_LIBRARY}")
endif()

file(GLOB_RECURSE IMGUI_SOURCES "ext/imgui/*.cpp" "ext/imgui/*.h")
add_library(imgui "${IMGUI_SOURCES}")

target_link_libraries(mirage PUBLIC "pthread" "glfw" "imgui")

target_compile_definitions(mirage PUBLIC MIRAGE_PROJECT_ROOT="${CMAKE_SOURCE_DIR}")
