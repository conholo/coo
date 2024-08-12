message(STATUS "Configuring for macOS")

# Vulkan SDK setup
find_package(Vulkan REQUIRED)
message(STATUS "Found Vulkan: $ENV{VULKAN_SDK}")

# GLFW setup
find_package(glfw3 3.3 REQUIRED)
set(GLFW_LIB glfw)
message(STATUS "Found GLFW")

# TinyOBJ setup
if(NOT TINYOBJ_PATH)
    set(TINYOBJ_PATH ${CMAKE_SOURCE_DIR}/external/tiny_obj)
    message(STATUS "TINYOBJ_PATH not specified, using: ${TINYOBJ_PATH}")
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
        ${Vulkan_INCLUDE_DIRS}
        ${GLFW_INCLUDE_DIRS}
        ${GLM_PATH}/include
        /usr/local/include
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
        ${GLFW_LIB}
        ${Vulkan_LIBRARIES}
)

# Mac-specific compile options
target_compile_definitions(${PROJECT_NAME} PRIVATE MAC_OS)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.${PROJECT_NAME}
    MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
    MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
)