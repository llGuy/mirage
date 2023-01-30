cmake_minimum_required(VERSION "3.2")

project(mirage)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_MBCS -DGLM_ENABLE_EXPERIMENTAL -Wordered-compare-function-pointers")

file(GLOB_RECURSE MIRAGE_SOURCES "src/*.cpp" "src/*.hpp")
add_executable(mirage "${MIRAGE_SOURCES}")

include_directories("${CMAKE_SOURCE_DIR}/dep/stb")
include_directories("${CMAKE_SOURCE_DIR}/dep/glm")
include_directories("${CMAKE_SOURCE_DIR}/dep/imgui/include")
include_directories("${CMAKE_SOURCE_DIR}/dep/imgui-console/include")

find_package(Vulkan)

if (Vulkan_FOUND)
    message(STATUS "Found Vulkan package in system ${Vulkan_LIBRARY}")
    target_include_directories(mirage PUBLIC "${Vulkan_INCLUDE_DIRS}")
    target_link_libraries(mirage PUBLIC "${Vulkan_LIBRARY}")
endif()

target_link_libraries(mirage PUBLIC "pthread" "glfw")

target_compile_definitions(mirage PUBLIC MIRAGE_PROJECT_ROOT="${CMAKE_SOURCE_DIR}")
