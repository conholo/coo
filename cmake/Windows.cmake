message(STATUS "Configuring for Windows")

include(${CMAKE_SOURCE_DIR}/.env.cmake OPTIONAL)

# MinGW-specific optimizations
if(MINGW)
    add_compile_options(-O3 -march=native -mtune=native)
    add_link_options(-s)  # Strip symbols for faster linking

    # Enable Link Time Optimization
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

    # Use unity build
    set_target_properties(${PROJECT_NAME} PROPERTIES UNITY_BUILD ON)
endif()

# Precompiled headers
target_precompile_headers(${PROJECT_NAME} PRIVATE
        <vector>
        <string>
        <vulkan/vulkan.h>
        <GLFW/glfw3.h>
)

# Vulkan SDK setup
if(DEFINED VULKAN_SDK_PATH)
    set(Vulkan_INCLUDE_DIRS "${VULKAN_SDK_PATH}/Include")
    set(Vulkan_LIBRARIES "${VULKAN_SDK_PATH}/Lib")
    set(Vulkan_FOUND "True")
else()
    find_package(Vulkan REQUIRED)
    message(STATUS "Found Vulkan: $ENV{VULKAN_SDK}")
endif()

# GLFW setup
if(DEFINED GLFW_PATH)
    message(STATUS "Using GLFW path specified in .env at: ${GLFW_PATH}")
    set(GLFW_INCLUDE_DIRS "${GLFW_PATH}/include")
    if(MSVC)
        set(GLFW_LIB "${GLFW_PATH}/lib-vc2019")
    elseif(CMAKE_GENERATOR STREQUAL "MinGW Makefiles" OR CMAKE_GENERATOR STREQUAL "Ninja")
        set(GLFW_LIB "${GLFW_PATH}/lib-mingw-w64")
    endif()
else()
    find_package(glfw3 3.3 REQUIRED)
    set(GLFW_LIB glfw)
    message(STATUS "Found GLFW")
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
        ${Vulkan_INCLUDE_DIRS}
        ${GLFW_INCLUDE_DIRS}
        ${GLM_PATH}/include
)

# Link directories
target_link_directories(${PROJECT_NAME} PRIVATE
        ${Vulkan_LIBRARIES}
        ${GLFW_LIB}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
        ${GLFW_LIB}/libglfw3.a
        ${Vulkan_LIBRARIES}/vulkan-1.lib
        ${SHADERC_LIBRARIES}
        ${SPIRV_CROSS_LIBRARIES}
)

# MinGW-specific flags
if(USE_MINGW)
    target_link_libraries(${PROJECT_NAME} PRIVATE
            -static-libgcc
            -static-libstdc++
            -lwsock32
            -lws2_32
            -lbcrypt
    )
endif()