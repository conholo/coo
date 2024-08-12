if(WIN32)
    set(SPIRV_CROSS_PATH "C:/dev/cpp/libs/SPIRV-Cross")

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "Using debug builds for shaderc and spirv-cross on Windows")
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
        message(STATUS "Using release builds for shaderc and spirv-cross on Windows")
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

elseif(APPLE)
    message(STATUS "Setting up shaderc and spirv-cross for macOS")

    find_library(SHADERC_LIB
            NAMES shaderc_shared
            PATHS /usr/local/Cellar/shaderc/2024.0/lib
            NO_DEFAULT_PATH
    )

    find_library(SPIRV_CROSS_C_LIB
            NAMES spirv-cross-c
            PATHS /usr/local/Cellar/spirv-cross/1.3.290.0/lib
            NO_DEFAULT_PATH
    )

    find_library(SPIRV_CROSS_CORE_LIB
            NAMES spirv-cross-core
            PATHS /usr/local/Cellar/spirv-cross/1.3.290.0/lib
            NO_DEFAULT_PATH
    )

    find_library(SPIRV_CROSS_GLSL_LIB
            NAMES spirv-cross-glsl
            PATHS /usr/local/Cellar/spirv-cross/1.3.290.0/lib
            NO_DEFAULT_PATH
    )

    find_library(SPIRV_CROSS_CPP_LIB
            NAMES spirv-cross-cpp
            PATHS /usr/local/Cellar/spirv-cross/1.3.290.0/lib
            NO_DEFAULT_PATH
    )

    set(SHADERC_LIBRARIES ${SHADERC_LIB})
    set(SPIRV_CROSS_LIBRARIES
            ${SPIRV_CROSS_C_LIB}
            ${SPIRV_CROSS_CORE_LIB}
            ${SPIRV_CROSS_GLSL_LIB}
            ${SPIRV_CROSS_CPP_LIB}
    )

    set(SHADERC_INCLUDE_DIRS "/usr/local/Cellar/shaderc/2024.0/include")
    set(SPIRV_CROSS_INCLUDE_DIRS "/usr/local/Cellar/spirv-cross/1.3.290.0/include")

else()
    message(FATAL_ERROR "Unsupported platform for SPIRV and Shaderc setup")
endif()

# Common setup for both platforms
target_include_directories(${PROJECT_NAME} PRIVATE ${SHADERC_INCLUDE_DIRS} ${SPIRV_CROSS_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${SHADERC_LIBRARIES} ${SPIRV_CROSS_LIBRARIES})