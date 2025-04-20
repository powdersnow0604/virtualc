#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>

// Function to copy files from installation directory to environment bin
void copy_files_to_env_bin(const std::string& env_bin_dir) {
    // Source directory for files to copy - use the CMake-defined path
#ifdef VIRTUALC_BIN_DIR
    const char* source_dir = VIRTUALC_BIN_DIR;
#else
    // Fallback for when not defined during compilation
    const char* source_dir = "/usr/local/bin/virtualcdir";
#endif
    
    // Copy files from VIRTUALC_BIN_DIR to env_name/bin
    std::filesystem::path src_path(source_dir);
    std::filesystem::path dst_path(env_bin_dir);
    
    // Create destination directory if it doesn't exist
    if (!std::filesystem::exists(dst_path)) {
        std::filesystem::create_directories(dst_path);
    }
    
    //std::cout << "Copying helper files from " << source_dir << " to " << env_bin_dir << std::endl;
    
    // Check if source directory exists
    if (!std::filesystem::exists(src_path)) {
        std::cerr << "Error: Source directory " << source_dir << " does not exist." << std::endl;
        return;
    }
    
    bool files_copied = false;
    
    // Copy all files from source to destination
    for (const auto& entry : std::filesystem::directory_iterator(src_path)) {
        std::filesystem::path dst_file = dst_path / entry.path().filename();
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            if (filename == "gcc" || filename == "g++" || filename == "icl") {
                try {
                    //std::cout << "Copying " << filename << "..." << std::endl;
                    std::filesystem::copy_file(entry.path(), dst_file, 
                        std::filesystem::copy_options::overwrite_existing);
                    // Set executable permissions for the copied files
                    chmod(dst_file.c_str(), 0755);
                    files_copied = true;
                } catch (const std::exception& e) {
                    std::cerr << "Error copying " << filename << ": " << e.what() << std::endl;
                }
            }
        }
    }
    
    // if (files_copied) {
    //     std::cout << "Helper files copied successfully." << std::endl;
    // } else {
    //     std::cout << "No helper files were copied." << std::endl;
    // }
    
    // Create an empty iclargs.json file in the virtual environment
    std::ofstream iclargs_file(dst_path / "iclargs.json");
    if (iclargs_file) {
        // Initialize with default library data pointing to virtual environment
        std::string virtual_env_path = env_bin_dir.substr(0, env_bin_dir.length() - 4); // Remove "/bin" from env_bin_dir
        std::string json_content = "{\n"
            "    \"libraries\": {\n"
            "        \"__default__\": {\n"
            "            \"includes\": [\n"
            "                \"" + virtual_env_path + "/include\"\n"
            "            ],\n"
            "            \"lib_names\": [\n"
            "\n"
            "            ],\n"
            "            \"lib_paths\": [\n"
            "                \"" + virtual_env_path + "/lib\"\n"
            "            ]\n"
            "        }\n"
            "    }\n"
            "}";
        
        iclargs_file << json_content << std::endl;
        iclargs_file.close();
    } else {
        std::cerr << "Error creating iclargs.json file in " << env_bin_dir << std::endl;
    }
    
    // Create a config file for additional pkg-config paths
    std::ofstream pkgconfig_file(dst_path / "pkgconfig.conf");
    if (pkgconfig_file) {
        pkgconfig_file << "# Additional pkg-config paths for this virtual environment" << std::endl;
        pkgconfig_file << "# Add one path per line. Lines starting with # are comments" << std::endl;
        pkgconfig_file << "# Example:" << std::endl;
        pkgconfig_file << "# /some/custom/path/lib/pkgconfig" << std::endl;
        pkgconfig_file.close();
    } else {
        std::cerr << "Error creating pkgconfig.conf file in " << env_bin_dir << std::endl;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <env name>\n", argv[0]);
        return 1;
    }

    // Create env_name from argv[1]
    std::filesystem::path env_path = std::filesystem::absolute(argv[1]);
    std::string env_name = env_path.filename().string();

    // Get the default value for VIRTUALC_VIRTUAL_LIB
    #ifdef VIRTUALC_VIRTUAL_LIB_DEFAULT
        const char* virtual_lib_default = VIRTUALC_VIRTUAL_LIB_DEFAULT;
    #else
        // Fallback for when not defined during compilation
        const char* virtual_lib_default = "true";
    #endif

    char buffer[4096];
    const char* activate_script =
        "# This file must be used with \"source bin/activate\" *from bash*\n"
        "# You cannot run it directly\n"
        "\n"
        "deactivate () {\n"
        "    # reset old environment variables\n"
        "    if [ -n \"${_OLD_VIRTUAL_PATH:-}\" ] ; then\n"
        "        PATH=\"${_OLD_VIRTUAL_PATH:-}\"\n"
        "        export PATH\n"
        "        unset _OLD_VIRTUAL_PATH\n"
        "    fi\n"
        "\n"
        "    # Remove GCC and G++ paths\n"
        "    unset _GCC_PATH\n"
        "    unset _GXX_PATH\n"
        "\n"
        "    # Unset the virtual environment library flag\n"
        "    unset VIRTUALC_VIRTUAL_LIB\n"
        "    if [ -n \"${_OLD_PKG_CONFIG_LIBDIR_PATH:-}\" ] ; then\n"
        "        PKG_CONFIG_LIBDIR=\"${_OLD_PKG_CONFIG_LIBDIR_PATH:-}\"\n"
        "        export PKG_CONFIG_LIBDIR\n"
        "        unset _OLD_PKG_CONFIG_LIBDIR_PATH\n"
        "    fi\n"
        "\n"
        "    # Call hash to forget past commands. Without forgetting\n"
        "    # past commands the $PATH changes we made may not be respected\n"
        "    hash -r 2> /dev/null\n"
        "\n"
        "    if [ -n \"${_OLD_VIRTUAL_PS1:-}\" ] ; then\n"
        "        PS1=\"${_OLD_VIRTUAL_PS1:-}\"\n"
        "        export PS1\n"
        "        unset _OLD_VIRTUAL_PS1\n"
        "    fi\n"
        "\n"
        "    unset VIRTUAL_ENV\n"
        "    unset VIRTUAL_ENV_PROMPT\n"
        "    if [ ! \"${1:-}\" = \"nondestructive\" ] ; then\n"
        "    # Self destruct!\n"
        "        unset -f deactivate\n"
        "    fi\n"
        "}\n"
        "\n"
        "# unset irrelevant variables\n"
        "deactivate nondestructive\n"
        "\n"
        "# Capture GCC and G++ paths\n"
        "export _GCC_PATH=$(which gcc)\n"
        "export _GXX_PATH=$(which g++)\n"
        "\n"
        "# on Windows, a path can contain colons and backslashes and has to be converted:\n"
        "if [ \"${OSTYPE:-}\" = \"cygwin\" ] || [ \"${OSTYPE:-}\" = \"msys\" ] ; then\n"
        "    # transform D:\\path\\to\\venv to /d/path/to/venv on MSYS\n"
        "    # and to /cygdrive/d/path/to/venv on Cygwin\n"
        "    export VIRTUAL_ENV=$(cygpath \"%s\")\n"
        "else\n"
        "    # use the path as-is\n"
        "    export VIRTUAL_ENV=%s\n"
        "fi\n"
        "\n"
        "# Set the virtual environment library flag\n"
        "export VIRTUALC_VIRTUAL_LIB=%s\n"
        "\n"
        "# Set PKG_CONFIG_LIBDIR to include the virtual environment's pkgconfig directory\n"
        "export _OLD_PKG_CONFIG_LIBDIR_PATH=\"$PKG_CONFIG_LIBDIR\"\n"
        "unset PKG_CONFIG_LIBDIR\n"
        "\n"
        "_OLD_VIRTUAL_PATH=\"$PATH\"\n"
        "PATH=\"$VIRTUAL_ENV/bin:$PATH\"\n"
        "export PATH\n"
        "\n"
        "if [ -z \"${VIRTUAL_ENV_DISABLE_PROMPT:-}\" ] ; then\n"
        "    _OLD_VIRTUAL_PS1=\"${PS1:-}\"\n"
        "    PS1='(%s) '\"${PS1:-}\"\n"
        "    export PS1\n"
        "    VIRTUAL_ENV_PROMPT='(%s) '\n"
        "    export VIRTUAL_ENV_PROMPT\n"
        "fi\n"
        "\n"
        "# Call hash to forget past commands. Without forgetting\n"
        "# past commands the $PATH changes we made may not be respected\n"
        "hash -r 2> /dev/null\n";
    
    snprintf(buffer, sizeof(buffer), activate_script, 
             env_path.string().c_str(), 
             env_path.string().c_str(), 
             virtual_lib_default,
             env_name.c_str(), 
             env_name.c_str());

    if (mkdir(argv[1], 0755) == -1)
    {
        perror("mkdir");
        return 1;
    }

    chdir(argv[1]);

    if (mkdir("bin", 0755) == -1)
    {
        perror("mkdir");
        return 1;
    }

    // Create lib/pkgconfig directory for virtual environment
    if (mkdir("lib", 0755) == -1)
    {
        perror("mkdir lib");
        return 1;
    }

    if (mkdir("include", 0755) == -1)
    {
        perror("mkdir include");
        return 1;
    }
    
    if (mkdir("lib/pkgconfig", 0755) == -1)
    {
        perror("mkdir lib/pkgconfig");
        return 1;
    }

    if (mkdir("lib/cmake", 0755) == -1)
    {
        perror("mkdir lib/cmake");
        return 1;
    }

    FILE *activate = fopen("bin/activate", "wb");
    if (activate == NULL)
    {
        perror("fopen");
        return 1;
    }
    if (fwrite(buffer, 1, strlen(buffer), activate) != strlen(buffer))
    {
        perror("fwrite");
        fclose(activate);
        return 1;
    }
    fclose(activate);

    // Set permissions for activate script
    if (chmod("bin/activate", 0755) == -1)
    {
        perror("chmod");
        return 1;
    }
    
    // Create .iclignore file and write specified lines
    std::ofstream iclignore_file("bin/.iclignore");
    if (!iclignore_file) {
        std::cerr << "Error creating .iclignore file" << std::endl;
        return 1;
    }
    iclignore_file << "/usr/local/include" << std::endl;
    iclignore_file << "/usr/local/lib" << std::endl;

    // Add virtual environment paths to .iclignore
    iclignore_file << std::filesystem::absolute("include").string() << std::endl;
    iclignore_file << std::filesystem::absolute("lib").string() << std::endl;

    iclignore_file.close(); // Close the file after writing

    // Copy helper files from installation directory
    copy_files_to_env_bin(std::filesystem::absolute("bin").string());

    printf("Virtual environment '%s' created successfully.\n", argv[1]);
    printf("To activate, run: source %s/bin/activate\n", env_path.string().c_str());

    return 0;
}