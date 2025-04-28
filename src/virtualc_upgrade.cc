#include "virtualc_upgrade.h"

// Function to upgrade the libs directory
int upgrade_libs_main() {
    std::cout << "Upgrading library scripts..." << std::endl;
    
    // Create a temporary directory
    std::string temp_dir = "/tmp/vc_upgrade_" + std::to_string(std::time(nullptr));
    std::string mkdir_cmd = "mkdir -p " + temp_dir;
    if (system(mkdir_cmd.c_str()) != 0) {
        std::cerr << "Error: Failed to create temporary directory" << std::endl;
        return 1;
    }
    
    // Clone the repository
    std::string clone_cmd = "git clone https://github.com/powdersnow0604/linux_scripts.git " + temp_dir;
    std::cout << "Cloning repository..." << std::endl;
    if (system(clone_cmd.c_str()) != 0) {
        std::cerr << "Error: Failed to clone repository" << std::endl;
        return 1;
    }
    
    // Check for authorize.sh in the repository
    std::string auth_script_path = temp_dir + "/authorize.sh";
    if (!std::filesystem::exists(auth_script_path)) {
        std::cerr << "Error: authorize.sh not found in repository" << std::endl;
        return 1;
    }
    
    // Make the authorization script executable
    std::cout << "Making authorize.sh executable..." << std::endl;
    std::string chmod_cmd = "chmod +x " + auth_script_path;
    if (system(chmod_cmd.c_str()) != 0) {
        std::cerr << "Error: Failed to make authorization script executable" << std::endl;
        return 1;
    }
    
    // Execute the authorization script with sudo
    std::cout << "Executing authorization script..." << std::endl;
    std::string sudo_cmd = "sudo " + auth_script_path;
    if (system(sudo_cmd.c_str()) != 0) {
        std::cerr << "Error: Failed to execute authorization script" << std::endl;
        return 1;
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
        }
    }
    
    // Create libs directory
    std::string mkdir_libs_cmd = "sudo mkdir -p " + libs_dir;
    if (system(mkdir_libs_cmd.c_str()) != 0) {
        std::cerr << "Error: Failed to create libs directory" << std::endl;
        return 1;
    }
    
    // Copy the libs directory from the cloned repository
    std::string cp_cmd = "sudo cp -r " + temp_dir + "/libs/* " + libs_dir;
    if (system(cp_cmd.c_str()) != 0) {
        std::cerr << "Error: Failed to copy libs directory" << std::endl;
        return 1;
    }
    
    // Clean up
    std::string cleanup_cmd = "rm -rf " + temp_dir;
    if (system(cleanup_cmd.c_str()) != 0) {
        std::cerr << "Warning: Failed to clean up temporary directory" << std::endl;
    }
    
    std::cout << "Library scripts upgraded successfully." << std::endl;
    return 0;
} 