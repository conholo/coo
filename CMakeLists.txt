cmake_minimum_required(VERSION 3.11.0)
set(NAME coo)
project(${NAME})
set(CMAKE_CXX_STANDARD 20)

# Common setup code here
message(STATUS "Using ${CMAKE_GENERATOR}")

# TinyOBJ setup
if (NOT TINYOBJ_PATH)
    set(TINYOBJ_PATH ${CMAKE_SOURCE_DIR}/third_party/)
    message(STATUS "TINYOBJ_PATH not specified in .env.cmake, using: ${TINYOBJ_PATH}")
endif ()

# Gather source files
file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES}
        src/vulkan/vulkan_material_layout.cpp
        src/core/scene.cpp)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/build")

# Include platform-specific CMake file
if (WIN32)
    include(CMakeLists_Windows.txt)
elseif (UNIX AND APPLE)
    include(CMakeLists_Mac.txt)
else ()
    message(FATAL_ERROR "Unsupported platform")
endif ()

# Common include directories
target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/include
        ${TINYOBJ_PATH}
)

# Shader compilation
find_program(GLSL_VALIDATOR glslangValidator HINTS
        ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}
        /usr/bin
        /usr/local/bin
        ${VULKAN_SDK_PATH}/Bin
        ${VULKAN_SDK_PATH}/Bin32
        $ENV{VULKAN_SDK}/Bin/
        $ENV{VULKAN_SDK}/Bin32/
)

# Get all .vert and .frag files in shaders directory
file(GLOB_RECURSE GLSL_SOURCE_FILES
        "${PROJECT_SOURCE_DIR}/assets/shaders/*.frag"
        "${PROJECT_SOURCE_DIR}/assets/shaders/*.vert"
        "${PROJECT_SOURCE_DIR}/assets/shaders/*.comp"
)

foreach (GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${PROJECT_SOURCE_DIR}/assets/shaders/${FILE_NAME}.spv")
    add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV} -g
            DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach (GLSL)

add_custom_target(
        Shaders
        DEPENDS ${SPIRV_BINARY_FILES}
)

# Make sure shaders are built before the main target
add_dependencies(${PROJECT_NAME} Shaders)

# Set the working directory for Visual Studio
if (MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif ()

# Print status messages
message(STATUS "Project '${PROJECT_NAME}' configured.")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")