#!/bin/bash

# Exit on error
set -e

# Print commands as they are executed
#set -x

# Check if cmake, pkg-config installed
if ! command -v cmake &> /dev/null; then
    echo "cmake could not be found"
    # Ask user if they want to install cmake
    read -p "Do you want to install cmake? (y/n): " answer
    if [ "$answer" == "y" ] || [ "$answer" == "Y" ]; then
        sudo apt-get install cmake
    else
        exit 1
    fi
fi

if ! command -v pkg-config &> /dev/null; then
    echo "pkg-config could not be found"
    # Ask user if they want to install pkg-config
    read -p "Do you want to install pkg-config? (y/n): " answer
    if [ "$answer" == "y" ] || [ "$answer" == "Y" ]; then
        sudo apt-get install pkg-config
    else
        exit 1
    fi
fi

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
if [ -d build ]; then
    read -p "Build directory already exists. Do you want to delete it? (y/n): " answer
    if [ "$answer" == "y" ] || [ "$answer" == "Y" ]; then
        rm -rf build
    fi
fi
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