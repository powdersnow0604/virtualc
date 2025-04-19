#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::string env_bin_dir = std::string(std::getenv("VIRTUAL_ENV")) + "/bin";

std::string get_compiler_command() {
    // Get the original g++ path from the environment variable
    char* gxx_path = std::getenv("_GXX_PATH");
    if (!gxx_path) {
        std::cerr << "Error: _GXX_PATH environment variable not set" << std::endl;
        exit(1);
    }
    return gxx_path;
}

std::vector<std::string> get_iclargs() {
    std::vector<std::string> args;
    
    // Try to open the JSON file
    std::ifstream args_file(env_bin_dir + "/iclargs.json");
    if (!args_file.is_open()) {
        // File doesn't exist or can't be opened
        return args;
    }
    
    // Try to parse the JSON
    json iclargs;
    try {
        args_file >> iclargs;
    } catch (const json::parse_error&) {
        // Invalid JSON
        return args;
    }
    
    // Extract all the includes, lib paths, and library names
    if (iclargs.contains("libraries")) {
        for (const auto& [lib_name, lib_data] : iclargs["libraries"].items()) {
            // Extract includes
            if (lib_data.contains("includes")) {
                for (const auto& include : lib_data["includes"]) {
                    args.push_back("-I" + include.get<std::string>());
                }
            }
            
            // Extract lib paths
            if (lib_data.contains("lib_paths")) {
                for (const auto& lib_path : lib_data["lib_paths"]) {
                    args.push_back("-L" + lib_path.get<std::string>());
                }
            }
            
            // Extract library names
            if (lib_data.contains("lib_names")) {
                for (const auto& lib_name : lib_data["lib_names"]) {
                    args.push_back("-l" + lib_name.get<std::string>());
                }
            }
        }
    }
    
    return args;
}

int main(int argc, char* argv[]) {
    // Get the original g++ command
    std::string gxx_cmd = get_compiler_command();
    
    // Get additional arguments from iclargs.json
    std::vector<std::string> iclargs = get_iclargs();
    
    // Construct the full command
    std::string cmd = gxx_cmd;
    
    // Add the original arguments
    for (int i = 1; i < argc; ++i) {
        cmd += " ";
        
        // If the argument contains spaces, quote it
        if (std::string(argv[i]).find(' ') != std::string::npos) {
            cmd += "\"" + std::string(argv[i]) + "\"";
        } else {
            cmd += argv[i];
        }
    }
    
    // Add the additional arguments
    for (const auto& arg : iclargs) {
        cmd += " ";
        
        // If the argument contains spaces, quote it
        if (arg.find(' ') != std::string::npos) {
            cmd += "\"" + arg + "\"";
        } else {
            cmd += arg;
        }
    }
    
    // Execute the command
    int result = std::system(cmd.c_str());
    
    // Return the exit code
    return WEXITSTATUS(result);
} 