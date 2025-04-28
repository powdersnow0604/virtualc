# VirtualC

VirtualC (or `vc`) is a package manager and virtual environment tool for C/C++ projects. It simplifies dependency management and project configuration by creating isolated environments for each project.

## Features

- **Project Initialization**: Create new C/C++ projects with proper configuration
- **Package Management**: Install, uninstall, and list packages
- **Multiple Package Support**: Install or uninstall multiple packages at once
- **Dependency Resolution**: Automatically installs required dependencies
- **Run Command**: Compile and run C/C++ files with proper dependencies
- **Environment Isolation**: Each project has its own isolated environment
- **Upgrade Command**: Keep library scripts up to date

## Installation

```bash
git clone https://github.com/powdersnow0604/virtualc.git
cd virtualc
./build.sh
```

## Usage

### Initialize a Project

```bash
vc init [path] [options]
```

Options:
- `-c, --compiler`: Set compiler path
- `-x, --cxx`: Use g++ as default compiler instead of gcc
- `-g, --global`: Install packages globally at specified path

### Install Packages

```bash
vc install <package1> [package2] [package3] ...
```

Packages should be registered to pkg-config  
Or the install script should exist in https://github.com/powdersnow0604/linux_scripts

### Uninstall Packages

```bash
vc uninstall <package1> [package2] [package3] ...
```

### List Installed Packages

```bash
vc list
```

### Run a C/C++ File

```bash
vc run <filename> [compiler_args]
```

### Upgrade Library Scripts

```bash
vc upgrade
```

### Clear Project

```bash
vc clear
```

## Project Structure

When you initialize a project with VirtualC, it creates:
- `cproject.toml`: Project configuration
- `.libpath`: Tracks installed packages
- `.venv/`: Directory containing installed packages
- `.ignorepath`: Path patterns to ignore
- `.verified`: Indicates dependencies have been verified

## Example Workflow

```bash
# Initialize a new project
vc init my_project

# Navigate to project directory
cd my_project

# Install some libraries
vc install libcurl json-c

# Create a C file that uses the libraries
echo '#include <stdio.h>\n#include <curl/curl.h>\nint main() {\n  printf("Hello VirtualC!\\n");\n  return 0;\n}' > main.c

# Run the file with dependencies
vc run main.c
```
