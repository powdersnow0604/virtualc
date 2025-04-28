#!/bin/bash
set -e

BUILD_DIR=build

# Build the project
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake ..
make -j$(nproc)

# Install the binary and download scripts
echo "Installing vc and downloading scripts..."
sudo cmake --install .

echo "Build complete"
echo "vc is now installed in /usr/local/bin"
echo "Installation scripts installed in /usr/local/bin/virtualcdir"

# Clean up
cd ..
rm -rf "$BUILD_DIR"