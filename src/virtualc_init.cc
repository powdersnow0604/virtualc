#include "virtualc_init.h"
#include <cxxopts.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

// Initialize the project
int init_main(int argc, char** argv) {
    try {
        cxxopts::Options options("virtualc init", "Initialize a new Virtual C project");
        options.add_options()
            ("h,help", "Print usage")
            ("c,compiler", "Specify the compiler (gcc, clang, msvc)", cxxopts::value<std::string>()->default_value("gcc"))
            ("p,path", "Specify the project path", cxxopts::value<std::string>()->default_value("."))
            ("g,global", "Install libraries globally", cxxopts::value<std::string>())
            ("x,cxx", "C++ mode");
        
        // Set up positional arguments
        options.parse_positional({"path"});
        options.positional_help("[PATH]");

        auto result = options.parse(argc, argv);
        if (result.count("help")) {
            std::cout << options.help() << std::endl;
            return 0;
        }
        std::string path = result["path"].as<std::string>();
        fs::path root = path.empty() ? fs::current_path() : fs::absolute(path);
        std::optional<std::string> compiler;
        std::optional<std::string> global_install;
        
        // Check if -cpp mode is set
        bool cpp_mode = result.count("cxx") > 0;
        
        // Handle compiler specification
        if (result.count("compiler")) {
            std::string compiler_path = result["compiler"].as<std::string>();
            if (!compiler_path.empty()) {
                compiler = compiler_path;
            } else {
                // Use default based on mode
                compiler = cpp_mode ? "g++" : "gcc";
            }
        } else if (cpp_mode) {
            // No compiler specified but cpp mode is on
            compiler = find_gpp_path();
        } else {
            compiler = find_gcc_path();
        }

        if (result.count("global")) global_install = result["global"].as<std::string>();

        if (!fs::exists(root)) {
            fs::create_directories(root);
        }
        create_project(root, compiler, global_install);
        std::cout << "Project initialized at: " << root << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
} 