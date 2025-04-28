#include "virtualc_run.h"
#include "virtualc_install.h"

// Implement run subcommand
int run_main(int argc, char** argv) {
    // Get absolute path to file
    fs::path file_path = fs::absolute(argv[0]);
    
    // 1. Extract parent directory and ensure it exists
    fs::path parent_dir = file_path.parent_path();
    if (!fs::exists(parent_dir)) {
        std::cerr << "Error: Parent directory '" << parent_dir << "' does not exist." << std::endl;
        return 1;
    }
    
    // 2. All behavior is relative to parent directory
    fs::current_path(parent_dir);
    
    // Set up paths
    fs::path tomlfile = parent_dir / "cproject.toml";
    fs::path libpath = parent_dir / ".libpath";
    fs::path verified = parent_dir / ".verified";
    
    // 3. Initialize project if it doesn't exist
    if (!fs::exists(tomlfile)) {
        std::cout << "Project not initialized. Initializing..." << std::endl;
        create_project(parent_dir, std::nullopt, std::nullopt);
    }
    
    // 4. Check .verified and dependencies
    if (!fs::exists(verified)) {
        std::cout << "Verifying dependencies..." << std::endl;
        
        // Get dependencies from cproject.toml
        std::vector<std::string> dependencies = get_dependencies(tomlfile);
        
        // Check each dependency is installed
        bool all_deps_installed = true;
        for (const auto& dep : dependencies) {
            if (!check_package_installed(libpath, dep)) {
                std::cout << "Dependency '" << dep << "' not installed. Installing..." << std::endl;
                // Install the missing dependency
                std::vector<std::string> dep_to_install = {dep};
                if (install_main(dep_to_install) != 0) {
                    std::cerr << "Failed to install dependency '" << dep << "'." << std::endl;
                    all_deps_installed = false;
                }
            }
        }
        
        if (all_deps_installed) {
            // Create .verified file
            std::ofstream verified_file(verified);
            verified_file.close();
        } else {
            std::cerr << "Not all dependencies could be installed." << std::endl;
            return 1;
        }
    }
    
    // 5. Run the file with compiler and arguments
    std::string compiler = get_compiler_path(tomlfile);
    std::vector<std::string> compiler_args = build_compiler_args(libpath);
    
    // Construct command
    std::string cmd = compiler;
    
    // Add the input file
    cmd += " \"" + file_path.string() + "\"";
    
    // Add the compiler arguments from .libpath
    for (const auto& arg : compiler_args) {
        if (arg.find(' ') != std::string::npos) {
            cmd += " \"" + arg + "\"";
        } else {
            cmd += " " + arg;
        }
    }
    
    // Add any additional arguments passed to the command
    for (int i = 1; i < argc; i++) {
        if (argv[i] && strlen(argv[i]) > 0) {
            if (strchr(argv[i], ' ') != nullptr) {
                cmd += " \"" + std::string(argv[i]) + "\"";
            } else {
                cmd += " " + std::string(argv[i]);
            }
        }
    }
    
    // Add standard output binary name
    cmd += " -o a.out";
    
    // Show command
    std::cout << "Executing: " << cmd << std::endl;
    
    // Execute the command
    int result = std::system(cmd.c_str());
    
    if (result == 0) {
        std::cout << "Compilation successful." << std::endl;
    } else {
        std::cerr << "Compilation failed." << std::endl;
    }
    
    return result;
} 