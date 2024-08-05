include(./.env.cmake OPTIONAL RESULT_VARIABLE LOCAL_ENV)
message(STATUS "Local .env.cmake: ${LOCAL_ENV}")

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

# Vulkan SDK setup
if (DEFINED VULKAN_SDK_PATH)
    set(Vulkan_INCLUDE_DIRS "${VULKAN_SDK_PATH}/Include")
    set(Vulkan_LIBRARIES "${VULKAN_SDK_PATH}/Lib")
    set(Vulkan_FOUND "True")
else ()
    find_package(Vulkan REQUIRED)
    message(STATUS "Found Vulkan: $ENV{VULKAN_SDK}")
endif ()

if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Could not find Vulkan library!")
else ()
    message(STATUS "Using vulkan lib at: ${Vulkan_LIBRARIES}")
endif ()

# GLFW setup
if (DEFINED GLFW_PATH)
    message(STATUS "Using GLFW path specified in .env at: ${GLFW_PATH}")
    set(GLFW_INCLUDE_DIRS "${GLFW_PATH}/include")
    if (MSVC)
        set(GLFW_LIB "${GLFW_PATH}/lib-vc2019")
    elseif (CMAKE_GENERATOR STREQUAL "MinGW Makefiles" OR CMAKE_GENERATOR STREQUAL "Ninja")
        set(GLFW_LIB "${GLFW_PATH}/lib-mingw-w64")
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

# TinyOBJ setup
if (NOT TINYOBJ_PATH)
    set(TINYOBJ_PATH ${CMAKE_SOURCE_DIR}/third_party/)
    message(STATUS "TINYOBJ_PATH not specified in .env.cmake, using: ${TINYOBJ_PATH}")
endif ()

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES}
        src/vulkan/vulkan_material_layout.cpp
        src/core/scene.cpp)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/build")

target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${Vulkan_INCLUDE_DIRS}
        ${GLFW_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}/include
        ${TINYOBJ_PATH}
        ${GLM_PATH}/include
)

target_link_directories(${PROJECT_NAME} PUBLIC
        ${Vulkan_LIBRARIES}
        ${GLFW_LIB}
)

if (WIN32)
    message(STATUS "CREATING BUILD FOR WINDOWS")

    if (USE_MINGW)
        target_include_directories(${PROJECT_NAME} PUBLIC ${MINGW_PATH}/include)
        target_link_directories(${PROJECT_NAME} PUBLIC ${MINGW_PATH}/lib)
    endif ()

    set(SPIRV_CROSS_PATH "C:/dev/cpp/libs/SPIRV-Cross")

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "Using the following debug builds: shaderc, spirv-cross")
        set(SHADERC_LIB_PATH "C:/dev/cpp/libs/shaderc/build-debug/libshaderc/libshaderc_combined.a")
        set(SPIRV_CROSS_BUILD_PATH "${SPIRV_CROSS_PATH}/build-debug")

        set(SPIRV_CROSS_LIBRARIES
                "${SPIRV_CROSS_BUILD_PATH}/libspirv-cross-cored.a"
                "${SPIRV_CROSS_BUILD_PATH}/libspirv-cross-glsld.a"
                "${SPIRV_CROSS_BUILD_PATH}/libspirv-cross-hlsld.a"
                "${SPIRV_CROSS_BUILD_PATH}/libspirv-cross-cppd.a"
                "${SPIRV_CROSS_BUILD_PATH}/libspirv-cross-reflectd.a"
                "${SPIRV_CROSS_BUILD_PATH}/libspirv-cross-msld.a"
                "${SPIRV_CROSS_BUILD_PATH}/libspirv-cross-utild.a"
        )
    else()
        message(STATUS "Using the following release builds: shaderc, spirv-cross")
        set(SHADERC_LIB_PATH "C:/dev/cpp/libs/shaderc/build/libshaderc/libshaderc_combined.a")

        set(SPIRV_CROSS_BUILD_PATH "${SPIRV_CROSS_PATH}/build")
        set(SPIRV_CROSS_LIBRARIES
                "${SPIRV_CROSS_BUILD_PATH}/libspirv-cross-core.a"
                "${SPIRV_CROSS_BUILD_PATH}/libspirv-cross-glsl.a"
                "${SPIRV_CROSS_BUILD_PATH}/libspirv-cross-hlsl.a"
                "${SPIRV_CROSS_BUILD_PATH}/libspirv-cross-cpp.a"
                "${SPIRV_CROSS_BUILD_PATH}/libspirv-cross-reflect.a"
                "${SPIRV_CROSS_BUILD_PATH}/libspirv-cross-msl.a"
                "${SPIRV_CROSS_BUILD_PATH}/libspirv-cross-util.a"
        )

    endif()

    set(SHADERC_INCLUDE_DIRS "C:/dev/cpp/libs/shaderc/libshaderc/include")
    set(SHADERC_LIBRARIES ${SHADERC_LIB_PATH})

    set(SPIRV_CROSS_INCLUDE_DIRS "${SPIRV_CROSS_PATH}/include")
    include_directories(${SHADERC_INCLUDE_DIRS} ${SPIRV_CROSS_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME}
            ${GLFW_LIB}/libglfw3.a
            ${Vulkan_LIBRARIES}/vulkan-1.lib
            ${SHADERC_LIBRARIES}
            ${SPIRV_CROSS_LIBRARIES}
    )

    # MinGW-specific flags
    if(USE_MINGW)
        target_link_libraries(${PROJECT_NAME}
                -static-libgcc
                -static-libstdc++
                -lwsock32
                -lws2_32
                -lbcrypt
        )
    endif()

elseif (UNIX)
    message(STATUS "CREATING BUILD FOR UNIX")
    target_link_libraries(${PROJECT_NAME}
            glfw
            ${Vulkan_LIBRARIES}/libvulkan.so
            ${Vulkan_LIBRARIES}/libshaderc_combined.a
            ${Vulkan_LIBRARIES}/libspirv-cross-core.a
            ${Vulkan_LIBRARIES}/libspirv-cross-glsl.a
            ${Vulkan_LIBRARIES}/libspirv-cross-hlsl.a
            ${Vulkan_LIBRARIES}/libspirv-cross-msl.a
            ${Vulkan_LIBRARIES}/libspirv-cross-cpp.a
            ${Vulkan_LIBRARIES}/libspirv-cross-reflect.a
            ${Vulkan_LIBRARIES}/libspirv-cross-util.a
    )
endif()

############## Build SHADERS #######################

# Find all vertex and fragment sources within shaders directory
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
