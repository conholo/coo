cmake_minimum_required(VERSION 3.11.0)
set(PROJECT_NAME coo)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable ccache
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

# Enable parallel compilation
include(ProcessorCount)
ProcessorCount(N)
if(NOT N EQUAL 0)
    set(CMAKE_BUILD_PARALLEL_LEVEL ${N})
endif()

# Use response files for Ninja
if(CMAKE_GENERATOR MATCHES "Ninja")
    set(CMAKE_NINJA_FORCE_RESPONSE_FILE 1 CACHE BOOL "Force Ninja to use response files.")
endif()

# Debug build settings
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-O0 -g)
endif()

# Function to add external dependencies
function(add_external_dependency NAME)
    add_subdirectory(external/${NAME})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${NAME})
    target_include_directories(${PROJECT_NAME} PRIVATE external/${NAME}/include)
endfunction()

# Gather source files
file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

# Set working directory for debugging
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/build")

# Include platform-specific CMake file
if(WIN32)
    include(cmake/Windows.cmake)
elseif(UNIX AND APPLE)
    include(cmake/Mac.cmake)
elseif(UNIX AND NOT APPLE)
    include(cmake/Linux.cmake)  # You may need to create this file for Linux support
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

# Common include directories (moved here to avoid duplication)
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/external/tiny_obj
        ${CMAKE_SOURCE_DIR}/external/stb
)

# spirv and shaderc
include(cmake/SpirvAndShaderc.cmake)

# Add external dependencies
add_external_dependency(spdlog)
add_external_dependency(imgui)

# Shader compilation
include(cmake/ShaderCompilation.cmake)

# Set the working directory for Visual Studio
if(MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()

# Print status messages
message(STATUS "Project '${PROJECT_NAME}' configured.")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Using ${CMAKE_GENERATOR}")