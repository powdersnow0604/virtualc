# VirtualC

VirtualC is a tool for creating isolated C/C++ development environments, similar to Python's virtualenv.

## Installation

```bash
mkdir build && cd build
cmake ..
make
sudo make install

# Alternatively

chmod +x build.sh
./build.sh
```

### Build Options

You can customize the build using CMake options:

```bash
# To set the default value of VIRTUALC_VIRTUAL_LIB to false
mkdir build && cd build
cmake -DVIRTUALC_VIRTUAL_LIB_DEFAULT=OFF ..
make
sudo make install
```

Available options:
- `VIRTUALC_VIRTUAL_LIB_DEFAULT`: Sets the default value for the `VIRTUALC_VIRTUAL_LIB` environment variable (ON/OFF, default: ON)

## Usage

### Create a new virtual environment

```bash
virtualc my_project
```

### Activate a virtual environment

```bash
source my_project/bin/activate
```

### Deactivate a virtual environment

```bash
deactivate
```

## Virtual Environment Library Management

VirtualC now supports virtual libraries. When the virtual environment is activated, libraries can be installed into the virtual environment itself rather than system-wide.

### How it works

When a virtual environment is activated:

1. The `VIRTUALC_VIRTUAL_LIB` environment variable is set to `true` by default (can be changed at build time)
2. The `PKG_CONFIG_PATH` is set to include `$VIRTUAL_ENV/lib/pkgconfig`
3. New libraries installed using `icl install` will be placed in the virtual environment

### Adding Additional pkg-config Paths

You can add additional pkg-config search paths by editing the `pkgconfig.conf` file in your virtual environment's bin directory:

```bash
# Activate your virtual environment
source my_project/bin/activate

# Edit the pkgconfig.conf file
nano $VIRTUAL_ENV/bin/pkgconfig.conf
```

Add one path per line (lines starting with # are comments):

```
/path/to/custom/lib/pkgconfig
/another/path/pkgconfig
```

### Installing Libraries

When `VIRTUALC_VIRTUAL_LIB` is true:
- pkg-config will only check .pc files under `$VIRTUAL_ENV/lib/pkgconfig` and any additional paths specified in pkgconfig.conf
- New libraries installed using `icl install` will be placed under the virtual environment path

When `VIRTUALC_VIRTUAL_LIB` is false:
- pkg-config behavior is unchanged, checking system-wide locations
- However, it will also check .pc files under `$VIRTUAL_ENV/lib/pkgconfig`

## Commands

### Install a library

```bash
icl install <library_name>
```

### Uninstall a library

```bash
icl uninstall <library_name>
```

### List installed libraries

```bash
icl list
``` 

### Add ignore path

```bash
# Added path will be ignored during installing
icl add <ignore path>
``` 

### Delete ignore path

```bash
# Delete ignore path
icl path <ignore path>
``` 