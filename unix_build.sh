#!/bin/bash

# Set environment variables for the correct SDK and toolchain paths
export SDKROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX14.5.sdk
export CMAKE_OSX_SYSROOT=$SDKROOT
export PATH="/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin:$PATH"

# Clear the previous build directory
rm -rf build

# Create and navigate to the build directory
mkdir -p build
cd build || exit

# Run CMake with the correct SDK path
cmake -DCMAKE_OSX_SYSROOT=$SDKROOT -S ../ -B .

# Build the project
make && make Shaders && ./vulkan_test

# Navigate back to the root directory
cd ..
