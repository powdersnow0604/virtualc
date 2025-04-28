#include "virtualc_install.h"

// Function to try installing a library using custom script
bool try_install_custom_library(const std::string& lib_name, const std::string& install_path) {
    // Convert library name to uppercase for directory search
    std::string lib_dir_name = to_uppercase(lib_name);
    std::string script_path = libs_dir + "/" + lib_dir_name + "/install_" + to_lowercase(lib_name) + ".sh";
    std::string lib_dir = libs_dir + "/" + lib_dir_name;
    std::string morevariable_path = lib_dir + "/.morevariable";
    
    // Check if the script exists
    std::ifstream script_file(script_path);
    if (!script_file) {
        std::cerr << "No custom installation script found for library '" << lib_name << "'" << std::endl;
        return false;
    }
    script_file.close();
    
    // Create a temporary directory for installation
    std::string temp_dir = "/tmp/vc_install_" + lib_name + "_" + std::to_string(std::time(nullptr));
    std::string mkdir_cmd = "mkdir -p " + temp_dir;
    if (execute_command(mkdir_cmd) != 0) {
        std::cerr << "Error: Failed to create temporary directory for installation" << std::endl;
        return false;
    }
    
    // Prepare command arguments
    std::string cmd_args;
    
    // Always ask for version number as the first argument
    std::string version;
    std::cout << "Enter version number for " << lib_name << ": ";
    std::getline(std::cin, version);

    if (version.empty()) {
        cmd_args = "0";
    } else {
        cmd_args = version;
    }
    
    // Check if .morevariable file exists
    std::ifstream morevariable_file(morevariable_path);
    if (morevariable_file) {
        std::cout << "Additional parameters needed for " << lib_name << ":" << std::endl;
        
        // Read each line as a description and prompt for input
        std::string description;
        while (std::getline(morevariable_file, description)) {
            if (!description.empty()) {
                std::string variable_value;
                std::cout << description << ": ";
                std::getline(std::cin, variable_value);
                
                // Add the value to command arguments
                cmd_args += " \"" + variable_value + "\"";
            }
        }
        morevariable_file.close();
    }
    
    // Add install prefix to command arguments for installation path
    std::cout << "Installing to: " << install_path << std::endl;
    cmd_args += " \"" + install_path + "\"";
    
    // Execute the installation script with all arguments in the temporary directory
    std::string cmd = "cd " + temp_dir + " && " + script_path + " " + cmd_args;
    std::cout << "Executing installation script in " << script_path << std::endl;
    
    int result = execute_command(cmd);
    
    // Clean up the temporary directory
    std::string cleanup_cmd = "rm -rf " + temp_dir;
    execute_command(cleanup_cmd);
    
    if (result != 0) {
        std::cerr << "Installation script failed with exit code " << result << std::endl;
        return false;
    }
    
    std::cout << "Installation script completed successfully." << std::endl;
    return true;
}

// Update install_main to handle multiple packages
int install_main(const std::vector<std::string>& packages) {
    int result = 0;
    
    for (const auto& pkg : packages) {
        std::cout << "Installing package: " << pkg << std::endl;
        fs::path cwd = fs::current_path();
        fs::path tomlfile = cwd / "cproject.toml";
        fs::path libpath = cwd / ".libpath";
        fs::path ignorepath = cwd / ".ignorepath";
        fs::path venv_dir = cwd / ".venv";

        // 1. Check project exists
        if (!fs::exists(tomlfile)) {
            std::cout << "Project not initialized. Initializing...\n";
            create_project(cwd, std::nullopt, std::nullopt);
        }
        // 2. Check libpath exists
        if (!fs::exists(libpath)) {
            std::ofstream(libpath); // create empty
        }
        // 3. Check if already installed
        if (is_package_installed(libpath, pkg)) {
            std::cout << "Package '" << pkg << "' is already installed.\n";
            continue;
        }
        // 4. Check pkg-config
        std::string exists_cmd = "pkg-config --exists " + pkg;
        int pkg_exists = std::system(exists_cmd.c_str());
        if (pkg_exists == 0) {
            // Get info
            std::string cflags = run_cmd("pkg-config --cflags " + pkg);
            std::string libs = run_cmd("pkg-config --libs " + pkg);
            std::string version = trim(run_cmd("pkg-config --modversion " + pkg));
            std::set<std::string> ignore = read_lines_set(ignorepath);
            std::vector<std::string> includes, libnames, libpaths;
            // Parse cflags
            std::istringstream iss(cflags + " " + libs);
            std::string token;
            while (iss >> token) {
                if (token.rfind("-I", 0) == 0) {
                    std::string path = token.substr(2);
                    if (!ignore.count(path)) includes.push_back(path);
                } else if (token.rfind("-L", 0) == 0) {
                    std::string path = token.substr(2);
                    if (!ignore.count(path)) libpaths.push_back(path);
                } else if (token.rfind("-l", 0) == 0) {
                    libnames.push_back(token.substr(2));
                }
            }
            append_libpath(libpath, pkg, version, includes, libnames, libpaths);
            add_dependency_toml(tomlfile, pkg, version);
            std::cout << "Installed '" << pkg << "' from pkg-config.\n";
            continue;
        }
        
        // 5. Check for install script in virtualcdir
        // Determine install path from cproject.toml - use absolute paths
        std::string default_install_path = (venv_dir / pkg).string(); // Default is now absolute
        std::string install_path = default_install_path; // Start with the default absolute path
        
        try {
            auto tbl = toml::parse_file(tomlfile.string());
            auto* proj = tbl.get_as<toml::table>("project");
            if (proj && proj->contains("installpath")) {
                auto install_node = proj->get("installpath");
                if (install_node && install_node->is_string()) {
                    std::string specified_path = install_node->value_or("");
                    if (!specified_path.empty()) {
                        // If a path is specified, make it absolute if it's not already
                        fs::path path_obj(specified_path);
                        if (path_obj.is_relative()) {
                            // Append the package name to the path
                            install_path = (cwd / specified_path / pkg).string();
                        } else {
                            // Already absolute, just append the package name
                            install_path = (path_obj / pkg).string();
                        }
                    }
                }
            }
        } catch (const std::exception& ex) {
            std::cerr << "Warning: Error reading installpath from cproject.toml: " << ex.what() << std::endl;
        }
        
        // Create the install directory to ensure parent directories exist
        fs::create_directories(fs::path(install_path).parent_path());
        
        // Try to install with custom script from virtualcdir
        if (try_install_custom_library(pkg, install_path)) {
            // After install, first try system pkg-config
            int pkg_exists2 = std::system(("pkg-config --exists " + pkg).c_str());
            if (pkg_exists2 == 0) {
                // Package registered globally, use system pkg-config
                std::string cflags = run_cmd("pkg-config --cflags " + pkg);
                std::string libs = run_cmd("pkg-config --libs " + pkg);
                std::string version2 = trim(run_cmd("pkg-config --modversion " + pkg));
                std::set<std::string> ignore = read_lines_set(ignorepath);
                std::vector<std::string> includes, libnames, libpaths;
                std::istringstream iss(cflags + " " + libs);
                std::string token;
                while (iss >> token) {
                    if (token.rfind("-I", 0) == 0) {
                        std::string path = token.substr(2);
                        if (!ignore.count(path)) includes.push_back(path);
                    } else if (token.rfind("-L", 0) == 0) {
                        std::string path = token.substr(2);
                        if (!ignore.count(path)) libpaths.push_back(path);
                    } else if (token.rfind("-l", 0) == 0) {
                        libnames.push_back(token.substr(2));
                    }
                }
                append_libpath(libpath, pkg, version2, includes, libnames, libpaths);
                add_dependency_toml(tomlfile, pkg, version2);
                std::cout << "Installed '" << pkg << "' using script.\n";
                continue;
            } else {
                // Try to find a local .pc file in the installation path
                fs::path pc_path = fs::path(install_path) / "lib" / "pkgconfig" / (pkg + ".pc");
                
                if (fs::exists(pc_path)) {
                    std::cout << "Found local pkg-config file: " << pc_path << std::endl;
                    
                    // Create custom pkg-config command that uses the specific .pc file
                    std::string pkg_config_dir = pc_path.parent_path().string();
                    std::string pkg_config_cmd = "PKG_CONFIG_PATH=\"" + pkg_config_dir + "\" pkg-config";
                    
                    // Get package information using the local .pc file
                    std::string cflags = run_cmd(pkg_config_cmd + " --cflags " + pkg);
                    std::string libs = run_cmd(pkg_config_cmd + " --libs " + pkg);
                    std::string version = trim(run_cmd(pkg_config_cmd + " --modversion " + pkg));
                    
                    if (version.empty()) version = "0"; // Default if no version found
                    
                    std::set<std::string> ignore = read_lines_set(ignorepath);
                    std::vector<std::string> includes, libnames, libpaths;
                    
                    // Parse the pkgconfig output
                    std::istringstream iss(cflags + " " + libs);
                    std::string token;
                    while (iss >> token) {
                        if (token.rfind("-I", 0) == 0) {
                            std::string path = token.substr(2);
                            if (!ignore.count(path)) includes.push_back(path);
                        } else if (token.rfind("-L", 0) == 0) {
                            std::string path = token.substr(2);
                            if (!ignore.count(path)) libpaths.push_back(path);
                        } else if (token.rfind("-l", 0) == 0) {
                            libnames.push_back(token.substr(2));
                        }
                    }
                    
                    // If no includes/libs were found, add standard paths
                    if (includes.empty() && fs::exists(fs::path(install_path) / "include")) {
                        includes.push_back((fs::path(install_path) / "include").string());
                    }
                    
                    if (libpaths.empty() && fs::exists(fs::path(install_path) / "lib")) {
                        libpaths.push_back((fs::path(install_path) / "lib").string());
                    }
                    
                    if (libnames.empty()) {
                        libnames.push_back(pkg); // Default to package name
                    }
                    
                    append_libpath(libpath, pkg, version, includes, libnames, libpaths);
                    add_dependency_toml(tomlfile, pkg, version);
                    std::cout << "Installed '" << pkg << "' using scripts.\n";
                    continue;
                } else {
                    std::cerr << "No pkg-config file found at " << pc_path << std::endl;
                    std::cerr << "Installation may have failed or package doesn't use pkg-config." << std::endl;
                    result = 1;
                    continue;
                }
            }
        } else {
            std::cerr << "No install script found and not available via pkg-config." << std::endl;
            result = 1;
            continue;
        }
    }
    
    return result;
} 