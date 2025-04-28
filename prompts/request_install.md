This file indicate how you implement subcommand 'install'
Refer oldsrc/icl.cc, behavior is similar to it
Implement 'install' command

### Input:

1. Package name

## Details

> **Steps**
> 
> 1. Check project exists (whether cproject.toml exists)
>     1. If not, initialize the project
> 2. Check libpath exists
>     1. If not, create it
> 3. Check if the package has already installed
>     1. If installed, notice to user and exit
> 4. Check the pkg-config whether the library is registered
>     1. If registered, get `include paths`, `lib paths`, `lib names`, `version`
>     2. Write them into .libpath
>     3. Modify dependeny part of cproject.toml
>     4. Installed Successfully, exit
> 5. Check whether the install script has exist
>     1. If exists, install with it
>     2. Package should be installed in `.venv/lib-name`
>     3. Or specified path during `init` subcommand by `-g` option
>         1. Which will be recorded in cproject.toml (installpath)
>     4. Let p = “specified path by -g” or .venv/lib-name
>         1. If installpath exist in cproject.toml use path specified by -g
>         2. If not use .venv/lib-name
>     5. `include paths`, `lib paths`, `lib names`, `version` ****should be installed under p
>     6. If installed successfully, modify dependeny part of cproject.toml
>     7. else print error message, exit
>     8. If not exists, exit

During writing `include paths`, `lib paths`, `lib names`, `version` into .libpath,

if the paths exists in .ignorepath, ignore them