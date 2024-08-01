#include(./.env.cmake OPTIONAL RESULT_VARIABLE LOCAL_ENV)
#message(STATUS "Local .env.cmake: ${LOCAL_ENV}")

cmake_minimum_required(VERSION 3.11.0)
set(NAME coo)
project(${NAME})
set(CMAKE_CXX_STANDARD 20)

message(STATUS "using ${CMAKE_GENERATOR}")
if (CMAKE_GENERATOR STREQUAL "MinGW Makefiles")
    if (NOT MINGW_PATH)
        message(FATAL_ERROR "MINGW_PATH not set in .env.cmake")
    endif ()
    set(USE_MINGW "True")
    set(CMAKE_C_COMPILER ${MINGW_PATH}/bin/gcc.exe)
    set(CMAKE_CXX_COMPILER ${MINGW_PATH}/bin/g++.exe)
endif ()

# 1. Set VULKAN_SDK_PATH in .env.cmake to target specific vulkan version
if (DEFINED VULKAN_SDK_PATH)
    set(Vulkan_INCLUDE_DIRS "${VULKAN_SDK_PATH}/Include") # 1.1 Make sure this include path is correct
    set(Vulkan_LIBRARIES "${VULKAN_SDK_PATH}/Lib") # 1.2 Make sure lib path is correct
    set(Vulkan_FOUND "True")
else ()
    find_package(Vulkan REQUIRED) # throws error if could not find Vulkan
    message(STATUS "Found Vulkan: $ENV{VULKAN_SDK}")
endif ()
if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Could not find Vulkan library!")
else ()
    message(STATUS "Using vulkan lib at: ${Vulkan_LIBRARIES}")
endif ()

# 2. Set GLFW_PATH in .env.cmake to target specific glfw
if (DEFINED GLFW_PATH)
    message(STATUS "Using GLFW path specified in .env at: ${GLFW_PATH}")
    set(GLFW_INCLUDE_DIRS "${GLFW_PATH}/include")
    if (MSVC)
        set(GLFW_LIB "${GLFW_PATH}/lib-vc2019") # 2.1 Update lib-vc2019 to use same version as your visual studio
    elseif (CMAKE_GENERATOR STREQUAL "MinGW Makefiles" OR CMAKE_GENERATOR STREQUAL "Ninja")
        message(STATUS "USING MINGW")
        set(GLFW_LIB "${GLFW_PATH}/lib-mingw-w64") # 2.1 make sure matches glfw mingw subdirectory
    endif ()
else ()
    find_package(glfw3 3.3 REQUIRED)
    set(GLFW_LIB glfw)
    message(STATUS "Found GLFW")
endif ()
if (NOT GLFW_LIB)
    message(FATAL_ERROR "Could not find glfw library!")
else ()
    message(STATUS "Using glfw lib at: ${GLFW_LIB}")
endif ()

# Find shaderc package
find_package(PkgConfig REQUIRED)
pkg_search_module(SHADERC REQUIRED shaderc)

# Include shaderc headers
include_directories(${SHADERC_INCLUDE_DIRS})

include_directories(external)

# If TINYOBJ_PATH not specified in .env.cmake, try fetching from git repo
if (NOT TINYOBJ_PATH)
    set(TINYOBJ_PATH ${CMAKE_SOURCE_DIR}/third_party/)
    message(STATUS "TINYOBJ_PATH not specified in .env.cmake, using: ${TINYOBJ_PATH}")
endif ()

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)
add_executable(${PROJECT_NAME}
        src/main.cpp
        src/core/camera.cpp
        src/vulkan/vulkan_instance.cpp
        src/vulkan/vulkan_logical_device.cpp
        src/vulkan/vulkan_physical_device.cpp
        src/vulkan/vulkan_context.cpp
        src/vulkan/vulkan_swapchain.cpp
        src/vulkan/vulkan_image.cpp
        src/core/window.cpp
        src/vulkan/vulkan_image_utils.cpp
        src/vulkan/vulkan_render_pass.cpp
        src/vulkan/vulkan_swapchain_renderer.cpp
        src/vulkan/vulkan_renderer.cpp
        src/vulkan/vulkan_deferred_renderer.cpp
        src/vulkan/vulkan_framebuffer.cpp
        src/vulkan/vulkan_graphics_pipeline.cpp
        src/vulkan/vulkan_shader.cpp
        src/vulkan/vulkan_shader_reflection.cpp
        src/vulkan/vulkan_buffer.cpp
        src/vulkan/vulkan_model.cpp
        src/vulkan/vulkan_material.cpp
        src/vulkan/vulkan_descriptors.cpp)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/build")

target_include_directories(${NAME} PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${Vulkan_INCLUDE_DIRS}
        ${GLFW_INCLUDE_DIRS}
        ${SHADERC_INCLUDE_DIRS})

if (WIN32)
    message(STATUS "CREATING BUILD FOR WINDOWS")

    if (USE_MINGW)
        target_include_directories(${PROJECT_NAME} PUBLIC ${MINGW_PATH}/include)
        target_link_directories(${PROJECT_NAME} PUBLIC ${MINGW_PATH}/lib)
    endif ()

    target_include_directories(${PROJECT_NAME} PUBLIC
            ${PROJECT_SOURCE_DIR}/include
            ${Vulkan_INCLUDE_DIRS}
            ${TINYOBJ_PATH}
            ${GLFW_INCLUDE_DIRS}
            ${GLM_PATH}/include
    )

    target_link_directories(${PROJECT_NAME} PUBLIC
            ${Vulkan_LIBRARIES}
            ${GLFW_LIB}
    )

    target_link_libraries(${PROJECT_NAME} glfw3 vulkan-1 shaderc)
elseif (UNIX)
    message(STATUS "CREATING BUILD FOR UNIX")
    target_include_directories(${PROJECT_NAME} PUBLIC
            ${PROJECT_SOURCE_DIR}/include
            ${TINYOBJ_PATH}
            ${SHADERC_INCLUDE_DIRS}
    )
    target_link_libraries(${PROJECT_NAME} glfw ${Vulkan_LIBRARIES} ${SHADERC_LIBRARIES})
endif ()


############## Build SHADERS #######################

# Find all vertex and fragment sources within shaders directory
# taken from VBlancos vulkan tutorial
# https://github.com/vblanco20-1/vulkan-guide/blob/all-chapters/CMakeLists.txt
find_program(GLSL_VALIDATOR glslangValidator HINTS
        ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}
        /usr/bin
        /usr/local/bin
        ${VULKAN_SDK_PATH}/Bin
        ${VULKAN_SDK_PATH}/Bin32
        $ENV{VULKAN_SDK}/Bin/
        $ENV{VULKAN_SDK}/Bin32/
)

# get all .vert and .frag files in shaders directory
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
