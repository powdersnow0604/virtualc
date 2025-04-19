#!/bin/bash

# Exit on error
set -e

# Print commands as they are executed
#set -x

# Default installation prefix
PREFIX=${PREFIX:-/usr/local}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    --prefix=*)
      PREFIX="${1#*=}"
      shift
      ;;
    *)
      echo "Unknown option: $1"
      echo "Usage: $0 [--prefix=/path/to/install]"
      exit 1
      ;;
  esac
done

echo "Building with installation prefix: $PREFIX"

# Create and enter the build directory
mkdir -p build
cd build

# Configure project with CMake
cmake .. -DCMAKE_INSTALL_PREFIX="$PREFIX"

# Build the project with all available CPU cores
cmake --build . -- -j$(nproc)

# Install to the specified prefix (might require sudo)
if [[ "$PREFIX" == "/usr/local" ]]; then
  echo "Installing to $PREFIX (requires root privileges)"
  sudo cmake --install .
else
  echo "Installing to $PREFIX"
  cmake --install .
fi

# Print directory contents for verification
#echo "Contents of $PREFIX/bin/virtualcdir:"
#ls -la "$PREFIX/bin/virtualcdir/"

# Go back to the root directory
cd ..

# Clean up build files
echo "Cleaning up build files..."

# Remove the build directory
rm -rf build

echo "Build and cleanup completed successfully!"
echo "Installed to $PREFIX"
echo "Run 'virtualc <env_name>' to create a virtual environment." 