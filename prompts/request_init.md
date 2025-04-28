I will reboot the program,
refering past sources under src directory, make files under src2 directory
Now only virtualc.cc needs
This file inform you how the subcommand 'init' should work
Before 'Options' section, i specified how directory should be
Files section show the example of each files
For cproject.toml, if -g option is not used, 'installpath' should not be written
For .libpath, they are the example of how information of libs should be and during init, .libpath should be empty file

Use:
c++17 for filesystem
https://github.com/marzer/tomlplusplus for parsing toml
https://github.com/jarro2783/cxxopts for parsing arguments


### Input:

1. PATH (can be empty)
2. Options

---

## Details

Initialize project to $PATH

```
$PATH
|- .venv
|- .gitignore
|- .ignorepath
|- .libpath
|- cproject.toml
|- README.md
```

### Options

`-c, --c-compiler`: set compiler to c compiler (gcc)

`-g, —globally-install`: if lib should be installed, install it in the specified path, need additional argument specifying the path

### Files

[cproject.toml]

```
[project]
compilerpath = "path-to-compiler"
installpath = "path-specified-by-g-option"
dependencies = ["lib1==version"]
```

[.libpath]

```
[lib-name]
version = "version"
includes = ["path-to-directory/.venv/lib-name/include"]
libnames = ["lib-name"]
libpaths = ["path-to-directory/.venv/lib-name/lib"]

[lib2-name]
...
```

[.gitignore]

```
# Build artifacts
*.o
*.obj
*.lo
*.a
*.lib
*.so
*.so.*
*.dll
*.dylib
*.exe
*.out
*.app

# Dependency files
*.d

# Generated files
*.log
*.tmp

# CMake
CMakeFiles/
CMakeCache.txt
cmake_install.cmake
Makefile
build/
out/

# Visual Studio
*.vcxproj*
*.sln
*.user
*.opensdf
*.sdf
*.pdb
*.idb
*.ipch
*.aps
*.ncb

# CodeBlocks
*.cbp
*.layout
*.depend

# Eclipse
.project
.cproject
.settings/

# Other IDEs
*.idea/
*.vscode/

# OS files
.DS_Store
Thumbs.db

# virtualc
.libpath
.verified
.venv
```

[.ignorepath]

```
/usr/local/lib
/usr/local/include
```