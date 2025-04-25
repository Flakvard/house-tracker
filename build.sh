#!/usr/bin/env bash

set -e  # Stop on first error

if [[ "$1" == "clean-cache" ]]; then
  echo "🧼 Cleaning CMake cache (preserving object files)..."
  rm -f build/CMakeCache.txt
  rm -rf build/CMakeFiles
  echo "✅ Cache cleaned. Re-run ./build.sh to configure again."
  exit 0
fi

# Optional: Use GCC 13 if available
if command -v g++-13 &> /dev/null; then
  export CC=gcc-13
  export CXX=g++-13
  echo "🔧 Using GCC 13"
else
  echo "⚠️  GCC 13 not found, using default compiler ($CXX)"
fi

# Create and enter build directory
mkdir -p build
cd build

# Run CMake configuration
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_STANDARD=20 \
  -DCMAKE_TOOLCHAIN_FILE=/home/marni/vcpkg/scripts/buildsystems/vcpkg.cmake

# Build the project
cmake --build . --config Release

echo "✅ Build completed successfully!"

