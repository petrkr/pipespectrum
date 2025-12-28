#!/bin/bash

set -e

echo "=== Building PipeSpectrum ==="

# Create build directory
if [ ! -d "build" ]; then
    mkdir build
fi

cd build

# Configure
echo "Configuring with CMake..."
cmake ..

# Build
echo "Building..."
make -j$(nproc)

echo ""
echo "Build complete! Run with: ./build/PipeSpectrum"
