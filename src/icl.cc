#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <nlohmann/json.hpp>
#include <cctype>  // For toupper
#include <memory>
#include <sstream>

using json = nlohmann::json;

// Default value for VIRTUALC_VIRTUAL_LIB if not defined at activation time
#ifdef VIRTUALC_VIRTUAL_LIB_DEFAULT
    const char* virtual_lib_default = VIRTUALC_VIRTUAL_LIB_DEFAULT;
#else
    // Fallback for when not defined during compilation
    const char* virtual_lib_default = "true";
#endif

// Get the virtual environment path
std::string env_bin_dir = std::string(std::getenv("VIRTUAL_ENV")) + "/bin";

// Define the directory where custom library scripts are stored
#ifdef VIRTUALC_BIN_DIR
    const char* source_dir = VIRTUALC_BIN_DIR;
#else
    // Fallback for when not defined during compilation
    const char* source_dir = "/usr/local/bin/virtualcdir";
#endif

// Define the directory where custom library scripts are stored
std::string libs_dir = std::string(source_dir) + "/libs";

// Define is_virtual_lib
bool is_virtual_lib = std::string(std::getenv("VIRTUALC_VIRTUAL_LIB")) == "true";


//====================================================================================

// Function to convert string to uppercase
std::string to_uppercase(const std::string& str) {
    std::string result = str;
    for (char& c : result) {
        c = std::toupper(c);
    }
    return result;
}

// Function to convert string to lowercase
std::string to_lowercase(const std::string& str) {
    std::string result = str;
    for (char& c : result) {
        c = std::tolower(c);
    }
    return result;
}

// Function to execute a shell command and get the result
int execute_command(const std::string& command) {
    return system(command.c_str());
}

// Function to check if a path is in the ignore list
bool is_path_ignored(const std::string& path) {
    std::ifstream ignore_file(env_bin_dir + "/.iclignore");
    if (!ignore_file) {
        // If we can't open the ignore file, assume nothing is ignored
        return false;
    }

    std::string ignore_path;
    while (std::getline(ignore_file, ignore_path)) {
        if (path == ignore_path) {
            return true;
        }
    }
    return false;
}

// Function to add a path to the ignore list
void add_ignore_path(const std::string& path) {
    // Check if path already exists in the ignore file
    if (is_path_ignored(path)) {
        std::cout << "Path '" << path << "' already exists in .iclignore" << std::endl;
        return;
    }
    
    // Append the path to the ignore file
    std::ofstream ignore_file(env_bin_dir + "/.iclignore", std::ios::app);
    if (!ignore_file) {
        std::cerr << "Error: Unable to open .iclignore file for writing" << std::endl;
        return;
    }
    
    ignore_file << path << std::endl;
    ignore_file.close();
    
    std::cout << "Added path '" << path << "' to .iclignore" << std::endl;
}

// Function to delete a path from the ignore list
void delete_ignore_path(const std::string& path) {
    // Read all paths from the ignore file
    std::ifstream in_file(env_bin_dir + "/.iclignore");
    if (!in_file) {
        std::cerr << "Error: Unable to open .iclignore file for reading" << std::endl;
        return;
    }
    
    std::vector<std::string> paths;
    std::string line;
    bool found = false;
    
    // Read all lines except the one to be deleted
    while (std::getline(in_file, line)) {
        if (line == path) {
            found = true;
            continue;
        }
        paths.push_back(line);
    }
    in_file.close();
    
    if (!found) {
        std::cout << "Path '" << path << "' not found in .iclignore" << std::endl;
        return;
    }
    
    // Write back the remaining paths
    std::ofstream out_file(env_bin_dir + "/.iclignore");
    if (!out_file) {
        std::cerr << "Error: Unable to open .iclignore file for writing" << std::endl;
        return;
    }
    
    for (const auto& p : paths) {
        out_file << p << std::endl;
    }
    out_file.close();
    
    std::cout << "Deleted path '" << path << "' from .iclignore" << std::endl;
}

// Function to make localized environment variables
std::string make_pkg_config_localized_env_vars() {
    std::string pkg_config_paths;

    const char* virtual_env = std::getenv("VIRTUAL_ENV");
    if (virtual_env) {
        std::string virtual_env_path = std::string(virtual_env);
        std::string pkgconfig_conf_path = virtual_env_path + "/bin/pkgconfig.conf";
        
        // Read additional pkg-config paths from config file
        std::ifstream pkgconfig_conf(pkgconfig_conf_path);
        std::vector<std::string> additional_paths;
        
        if (pkgconfig_conf.is_open()) {
            std::string line;
            while (std::getline(pkgconfig_conf, line)) {
                // Skip comments and empty lines
                if (!line.empty() && line[0] != '#') {
                    additional_paths.push_back(line);
                }
            }
            pkgconfig_conf.close();
        }
        // Add virtual environment's pkgconfig path first if VIRTUALC_VIRTUAL_LIB is true
        pkg_config_paths = virtual_env_path + "/lib/pkgconfig";
        
        // Add additional custom paths
        for (const auto& path : additional_paths) {
            pkg_config_paths += ":" + path;
        }   
    }

    if (is_virtual_lib) {
        return "PKG_CONFIG_LIBDIR=\"" + pkg_config_paths + "\"";
    } else {
        return "PKG_CONFIG_PATH=\"" + pkg_config_paths + "\":$PKG_CONFIG_PATH";
    }
}

// Function to make cmd for pkg-config
std::string make_pkg_config_cmd(const std::string& args) {
    return make_pkg_config_localized_env_vars() + " pkg-config " + args + " 2>/dev/null";
}

// Function to run pkg-config and get the output
bool run_pkg_config(const std::string& args, std::string& result) {
    
    // Get the virtual environment path
    std::string cmd = make_pkg_config_cmd(args);
    
    
    // Define a custom deleter to avoid the attributes warning
    struct PipeDeleter {
        void operator()(FILE* pipe) const {
            if (pipe) pclose(pipe);
        }
    };
    
    // Use the custom deleter instead of decltype(&pclose)
    std::unique_ptr<FILE, PipeDeleter> pipe(popen(cmd.c_str(), "r"));
    
    if (!pipe) {
        return false;
    }
    
    result.clear();
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe.get()) != nullptr) {
        result += buffer;
    }
    
    // Check if pkg-config returned an error (usually exit code 1)
    int exit_code = pclose(pipe.release());
    if (exit_code != 0) {
        return false;
    }
    
    // Remove trailing newline if present
    if (!result.empty() && result[result.length()-1] == '\n') {
        result.erase(result.length()-1);
    }
    
    return true;
}

// Function to parse pkg-config output into separate flags
void parse_pkg_config_output(const std::string& output, std::vector<std::string>& includes, 
                            std::vector<std::string>& libs, std::vector<std::string>& lib_names) {
    std::string current;
    for (size_t i = 0; i < output.length(); ++i) {
        if (output[i] == ' ' && !current.empty()) {
            if (current.substr(0, 2) == "-I") {
                // Remove the -I prefix to get the path
                std::string path = current.substr(2);
                if (!is_path_ignored(path)) {
                    includes.push_back(path);
                }
            } else if (current.substr(0, 2) == "-L") {
                // Remove the -L prefix to get the path
                std::string path = current.substr(2);
                if (!is_path_ignored(path)) {
                    libs.push_back(path);
                }
            } else if (current.substr(0, 2) == "-l") {
                // This is a library name
                lib_names.push_back(current.substr(2));
            }
            current.clear();
        } else {
            current += output[i];
        }
    }
    
    // Process the last element
    if (!current.empty()) {
        if (current.substr(0, 2) == "-I") {
            std::string path = current.substr(2);
            if (!is_path_ignored(path)) {
                includes.push_back(path);
            }
        } else if (current.substr(0, 2) == "-L") {
            std::string path = current.substr(2);
            if (!is_path_ignored(path)) {
                libs.push_back(path);
            }
        } else if (current.substr(0, 2) == "-l") {
            lib_names.push_back(current.substr(2));
        }
    }
}

// Function to try installing a library using custom script
bool try_install_custom_library(const std::string& lib_name) {
    
    // Get virtual environment path if needed
    std::string virtual_env_path;
    std::string install_prefix;
    
    if (is_virtual_lib) {
        // Get the virtual environment path
        const char* virtual_env = std::getenv("VIRTUAL_ENV");
        if (virtual_env) {
            virtual_env_path = std::string(virtual_env);
            install_prefix = virtual_env_path;
        } else {
            std::cerr << "Error: VIRTUAL_ENV not set but VIRTUALC_VIRTUAL_LIB is true" << std::endl;
            return false;
        }
    } else {
        install_prefix = "";
    }
    
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
    std::string temp_dir = "/tmp/icl_install_" + lib_name + "_" + std::to_string(std::time(nullptr));
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
    if (is_virtual_lib) {
        std::cout << "Installing to virtual environment: " << install_prefix << std::endl;
    }
    cmd_args += " \"" + install_prefix + "\"";
    
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

// Function to install a library
void install_library(const std::string& lib_name) {
    // Check if trying to install __default__ library
    if (lib_name == "__default__") {
        std::cerr << "Error: Cannot install a library named '__default__' as it is reserved for system use." << std::endl;
        return;
    }

    // Check if pkg-config knows about this library
    std::string cflags, libs;
    
    // Run pkg-config --exists to check if the package exists
    std::string check_cmd = "--exists " + lib_name;
    int pkg_exists = system(make_pkg_config_cmd(check_cmd).c_str());
    
    // If pkg-config doesn't find the library, try custom installation
    if (pkg_exists != 0) {
        std::cout << "Library '" << lib_name << "' not found by pkg-config, trying custom installation..." << std::endl;
        
        // Try to install with custom script
        bool custom_install_success = try_install_custom_library(lib_name);
        
        // If custom installation succeeded, check pkg-config again
        if (custom_install_success) {
            pkg_exists = system(make_pkg_config_cmd(check_cmd).c_str());
            if (pkg_exists != 0) {
                std::cerr << "Error: Library '" << lib_name << "' was not found by pkg-config after custom installation" << std::endl;
                return;
            }
        } else {
            std::cerr << "Error: Failed to install library '" << lib_name << "'" << std::endl;
            return;
        }
    }
    
    try {
        // Get the library information
        bool cflags_success = run_pkg_config("--cflags " + lib_name, cflags);
        bool libs_success = run_pkg_config("--libs " + lib_name, libs);
        
        // If either operation failed, something went wrong
        if (!cflags_success || !libs_success) {
            std::cerr << "Error: Failed to get complete information for library '" << lib_name << "'" << std::endl;
            return;
        }
        
        // Parse the output
        std::vector<std::string> includes, lib_paths, lib_names;
        parse_pkg_config_output(cflags, includes, lib_paths, lib_names);
        parse_pkg_config_output(libs, includes, lib_paths, lib_names);
        
        // Read the current json file
        json iclargs;
        std::ifstream args_file(env_bin_dir + "/iclargs.json");
        if (args_file.is_open()) {
            try {
                args_file >> iclargs;
            } catch (const json::parse_error&) {
                iclargs = json{};
            }
            args_file.close();
        }
        
        // Update the json
        if (!iclargs.contains("libraries")) {
            iclargs["libraries"] = json::object();
        }
        
        iclargs["libraries"][lib_name] = {
            {"includes", includes},
            {"lib_paths", lib_paths},
            {"lib_names", lib_names}
        };
        
        // Write it back
        std::ofstream out_file(env_bin_dir + "/iclargs.json");
        out_file << iclargs.dump(4);
        out_file.close();
        
        std::cout << "Library '" << lib_name << "' installed successfully." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error installing library '" << lib_name << "': " << e.what() << std::endl;
    }
}

// Function to uninstall a library
void uninstall_library(const std::string& lib_name) {
    // Prevent uninstallation of __default__ library
    if (lib_name == "__default__") {
        std::cerr << "Error: Cannot uninstall the '__default__' library as it is required by the system." << std::endl;
        return;
    }

    // Read the current json file
    json iclargs;
    std::ifstream args_file(env_bin_dir + "/iclargs.json");
    if (args_file.is_open()) {
        try {
            args_file >> iclargs;
        } catch (const json::parse_error&) {
            iclargs = json{};
        }
        args_file.close();
    }
    
    // Remove the library if it exists
    if (iclargs.contains("libraries") && iclargs["libraries"].contains(lib_name)) {
        iclargs["libraries"].erase(lib_name);
        
        // Write it back
        std::ofstream out_file(env_bin_dir + "/iclargs.json");
        out_file << iclargs.dump(4);
        out_file.close();
        
        std::cout << "Library '" << lib_name << "' uninstalled successfully." << std::endl;
    } else {
        std::cerr << "Library '" << lib_name << "' is not installed." << std::endl;
    }
}

// Function to list installed libraries
void list_libraries() {
    // Read the current json file
    json iclargs;
    std::ifstream args_file(env_bin_dir + "/iclargs.json");
    if (args_file.is_open()) {
        try {
            args_file >> iclargs;
        } catch (const json::parse_error&) {
            iclargs = json{};
        }
        args_file.close();
    }
    
    // List the libraries
    //if (iclargs.contains("libraries") && !iclargs["libraries"].empty()) {
    if (iclargs.contains("libraries") && iclargs["libraries"].size() > 1) {
        // bool has_user_libraries = false;
        
        // // Check if there are any libraries other than __default__
        // for (auto& [lib_name, lib_data] : iclargs["libraries"].items()) {
        //     if (lib_name != "__default__") {
        //         has_user_libraries = true;
        //         break;
        //     }
        // }
        
        //if (has_user_libraries) {
            std::cout << "Installed libraries:" << std::endl;
            
            for (auto& [lib_name, lib_data] : iclargs["libraries"].items()) {
                // Skip the __default__ library
                if (lib_name != "__default__") {
                    std::cout << "- " << lib_name << std::endl;
                }
            }
        //} else {
        //    std::cout << "No libraries installed." << std::endl;
        //}
    } else {
        std::cout << "No libraries installed." << std::endl;
    }
}

// Function to upgrade the libs directory
void upgrade_libs() {
    std::cout << "Upgrading library scripts..." << std::endl;
    
    // Create a temporary directory
    std::string temp_dir = "/tmp/icl_upgrade_" + std::to_string(std::time(nullptr));
    std::string mkdir_cmd = "mkdir -p " + temp_dir;
    if (system(mkdir_cmd.c_str()) != 0) {
        std::cerr << "Error: Failed to create temporary directory" << std::endl;
        return;
    }
    
    // Clone the repository
    std::string clone_cmd = "git clone https://github.com/powdersnow0604/linux_scripts.git " + temp_dir;
    std::cout << "Cloning repository..." << std::endl;
    if (system(clone_cmd.c_str()) != 0) {
        std::cerr << "Error: Failed to clone repository" << std::endl;
        return;
    }
    
    // Check for authorize.sh in the repository
    std::string auth_script_path = temp_dir + "/authorize.sh";
    if (!std::filesystem::exists(auth_script_path)) {
        std::cerr << "Error: authorize.sh not found in repository" << std::endl;
        return;
    }
    
    // Make the authorization script executable
    std::cout << "Making authorize.sh executable..." << std::endl;
    std::string chmod_cmd = "chmod +x " + auth_script_path;
    if (system(chmod_cmd.c_str()) != 0) {
        std::cerr << "Error: Failed to make authorization script executable" << std::endl;
        return;
    }
    
    // Execute the authorization script with sudo
    std::cout << "Executing authorization script..." << std::endl;
    std::string sudo_cmd = "sudo " + auth_script_path;
    if (system(sudo_cmd.c_str()) != 0) {
        std::cerr << "Error: Failed to execute authorization script" << std::endl;
        return;
    }
    
    // Remove existing libs directory if user confirms
    if (std::filesystem::exists(libs_dir)) {
        std::string user_input;
        std::cout << "Are you sure you want to remove the existing libs directory? (y/n): ";
        std::cin >> user_input;
        if (user_input == "y" || user_input == "Y") {
            std::string rm_cmd = "sudo rm -rf " + libs_dir;
            if (system(rm_cmd.c_str()) != 0) {
                std::cerr << "Warning: Failed to remove existing libs directory" << std::endl;
            }
        } else {
            std::cout << "Skipping removal of libs directory." << std::endl;
            //return;
        }
    }
    
    // Create libs directory
    std::string mkdir_libs_cmd = "sudo mkdir -p " + libs_dir;
    if (system(mkdir_libs_cmd.c_str()) != 0) {
        std::cerr << "Error: Failed to create libs directory" << std::endl;
        return;
    }
    
    // Copy the libs directory from the cloned repository
    std::string cp_cmd = "sudo cp -r " + temp_dir + "/libs/* " + libs_dir;
    if (system(cp_cmd.c_str()) != 0) {
        std::cerr << "Error: Failed to copy libs directory" << std::endl;
        return;
    }
    
    // Clean up
    std::string cleanup_cmd = "rm -rf " + temp_dir;
    if (system(cleanup_cmd.c_str()) != 0) {
        std::cerr << "Warning: Failed to clean up temporary directory" << std::endl;
    }
    
    std::cout << "Library scripts upgraded successfully." << std::endl;
}

void print_help() {
    std::cerr << "Usage: icl <command> [arguments]" << std::endl;
    std::cerr << "Commands:" << std::endl;
    std::cerr << "  install <library...>   Install one or more libraries" << std::endl;
    std::cerr << "  uninstall <library...> Uninstall one or more libraries" << std::endl;
    std::cerr << "  list                   List installed libraries" << std::endl;
    std::cerr << "  add <path...>          Add path(s) to .iclignore" << std::endl;
    std::cerr << "  delete <path...>       Delete path(s) from .iclignore" << std::endl;
    std::cerr << "  upgrade                Upgrade library scripts from repository" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help();
        return 1;
    }
    
    std::string command = argv[1];
    
    if (command == "install") {
        if (argc < 3) {
            std::cerr << "Error: No library specified for installation" << std::endl;
            return 1;
        }
        
        for (int i = 2; i < argc; ++i) {
            install_library(argv[i]);
        }
    } else if (command == "uninstall") {
        if (argc < 3) {
            std::cerr << "Error: No library specified for uninstallation" << std::endl;
            return 1;
        }
        
        for (int i = 2; i < argc; ++i) {
            uninstall_library(argv[i]);
        }
    } else if (command == "list") {
        list_libraries();
    } else if (command == "add") {
        if (argc < 3) {
            std::cerr << "Error: No path specified to add to .iclignore" << std::endl;
            return 1;
        }
        
        for (int i = 2; i < argc; ++i) {
            add_ignore_path(argv[i]);
        }
    } else if (command == "delete") {
        if (argc < 3) {
            std::cerr << "Error: No path specified to delete from .iclignore" << std::endl;
            return 1;
        }
        
        for (int i = 2; i < argc; ++i) {
            delete_ignore_path(argv[i]);
        }
    } else if (command == "upgrade") {
        upgrade_libs();
    } else if (command == "--help" || command == "-h") {
        print_help();
    } else {
        std::cerr << "Unknown command: " << command << std::endl;
        return 1;
    }
    
    return 0;
} 