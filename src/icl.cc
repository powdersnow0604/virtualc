#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <nlohmann/json.hpp>

using json = nlohmann::json;


std::string env_bin_dir = std::string(std::getenv("VIRTUAL_ENV")) + "/bin";


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

// Function to run pkg-config and get the output
bool run_pkg_config(const std::string& args, std::string& result) {
    std::string cmd = "pkg-config " + args + " 2>/dev/null";
    
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

// Function to install a library
void install_library(const std::string& lib_name) {
    // Check if pkg-config knows about this library
    std::string cflags, libs;
    
    // Run pkg-config --exists to check if the package exists
    std::string check_cmd = "pkg-config --exists " + lib_name;
    int pkg_exists = system(check_cmd.c_str());
    if (pkg_exists != 0) {
        std::cerr << "Error: Library '" << lib_name << "' not found by pkg-config" << std::endl;
        return;
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
    if (iclargs.contains("libraries") && !iclargs["libraries"].empty()) {
        std::cout << "Installed libraries:" << std::endl;
        for (auto& [lib_name, lib_data] : iclargs["libraries"].items()) {
            std::cout << "- " << lib_name << std::endl;
        }
    } else {
        std::cout << "No libraries installed." << std::endl;
    }
}

void print_help() {
    std::cerr << "Usage: icl <command> [arguments]" << std::endl;
    std::cerr << "Commands:" << std::endl;
    std::cerr << "  install <library...>   Install one or more libraries" << std::endl;
    std::cerr << "  uninstall <library...> Uninstall one or more libraries" << std::endl;
    std::cerr << "  list                   List installed libraries" << std::endl;
    std::cerr << "  add <path...>          Add path(s) to .iclignore" << std::endl;
    std::cerr << "  delete <path...>       Delete path(s) from .iclignore" << std::endl;
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
    } else if (command == "--help" || command == "-h") {
        print_help();
    } else {
        std::cerr << "Unknown command: " << command << std::endl;
        return 1;
    }
    
    return 0;
} 