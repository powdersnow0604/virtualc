Implement uninstall subcommand

### Input:

1. Package name

## Details

> **Steps**
> 
> 1. Check project exists (whether cproject.toml exists)
>     1. If not, initialize the project
> 2. Check libpath exists
>     1. If not, create it
> 3. Check if the package has installed (check .libpath)
>     1. If not, exit with error message
>     2. If exist, remove package information from .libpath and crpoject.toml
> 4. Check if ‘package-name’ directory has exists in `.venv`
>     1. If exist, remove that directory