Implement 'run' subcommand
It excutes compiler (e.g. gcc) for input Filename
Arguments for compiler is added using libs in virtual env

### Input:

1. Filename (path)


## Details

> **Steps**
> 
> 1. Extract the parent directory of the specified $Filename (path)
>     1. If parent directory does not exists, exit
> 2. All behavior is relative to the parent directory
> 3. If project not exist, initialize
> 4. If .verified not exists,
>     1. Check whether every packages specified in the `dependencies` of cproject.toml has exists in .libpath
>     2. If not, install uninstalled packages
>     3. Make .verified
> 5. Run $Filename (path) with compiler specified in cproject.toml with arguments from .libpath