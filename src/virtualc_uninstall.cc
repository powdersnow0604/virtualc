#include "virtualc_uninstall.h"

// Implement uninstall subcommand to handle multiple packages
int uninstall_main(const std::vector<std::string>& packages) {
    int result = 0;
    
    for (const auto& pkg : packages) {
        std::cout << "Uninstalling package: " << pkg << std::endl;
        fs::path cwd = fs::current_path();
        fs::path tomlfile = cwd / "cproject.toml";
        fs::path libpath = cwd / ".libpath";
        fs::path venv_dir = cwd / ".venv";
        fs::path pkg_dir = venv_dir / pkg;
        
        // 1. Check project exists
        if (!fs::exists(tomlfile)) {
            std::cout << "Project not initialized. Nothing to uninstall.\n";
            result = 1;
            continue;
        }
        
        // 2. Check libpath exists
        if (!fs::exists(libpath)) {
            std::cout << "No packages installed (.libpath not found).\n";
            result = 1;
            continue;
        }
        
        // 3. Check if package is installed and remove from .libpath
        if (!remove_package_from_libpath(libpath, pkg)) {
            std::cerr << "Error: Package '" << pkg << "' is not installed.\n";
            result = 1;
            continue;
        }
        
        // Remove from cproject.toml
        remove_dependency_toml(tomlfile, pkg);
        
        // 4. Check if package directory exists in .venv and remove it
        if (fs::exists(pkg_dir) && fs::is_directory(pkg_dir)) {
            try {
                std::cout << "Removing directory: " << pkg_dir << std::endl;
                fs::remove_all(pkg_dir);
            } catch (const std::exception& ex) {
                std::cerr << "Permission denied when removing package directory." << std::endl;
                std::cout << "Do you want to use sudo to remove the directory? (y/n): ";
                std::string response;
                std::getline(std::cin, response);
                
                if (response == "y" || response == "Y") {
                    std::string rm_cmd = "sudo rm -rf \"" + pkg_dir.string() + "\"";
                    int rm_result = std::system(rm_cmd.c_str());
                    if (rm_result != 0) {
                        std::cerr << "Warning: Failed to remove package directory using sudo." << std::endl;
                    } else {
                        std::cout << "Successfully removed package directory." << std::endl;
                    }
                } else {
                    std::cerr << "Warning: Package directory not removed. You may need to manually delete it." << std::endl;
                }
            }
        }
        
        std::cout << "Package '" << pkg << "' uninstalled successfully.\n";
    }
    
    return result;
} 