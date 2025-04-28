#include "virtualc_list.h"

// Function to list all installed packages
int list_packages_main() {
    fs::path cwd = fs::current_path();
    fs::path libpath = cwd / ".libpath";
    
    // Check if .libpath exists
    if (!fs::exists(libpath)) {
        std::cout << "No packages installed (missing .libpath file).\n";
        return 0;
    }
    
    // Read .libpath file to find installed packages
    std::ifstream file(libpath);
    if (!file) {
        std::cerr << "Error: Failed to open .libpath file.\n";
        return 1;
    }
    
    std::string line;
    std::vector<std::string> packages;
    std::string current_pkg;
    std::string version;
    
    // Parse the .libpath file
    while (std::getline(file, line)) {
        line = trim(line);
        
        // If line starts with [, it's a package name
        if (!line.empty() && line[0] == '[') {
            size_t end_bracket = line.find(']');
            if (end_bracket != std::string::npos) {
                current_pkg = line.substr(1, end_bracket - 1);
                version = ""; // Reset version
            }
        }
        // If line starts with "version", extract the version
        else if (!current_pkg.empty() && line.find("version") == 0) {
            size_t start_quote = line.find('"');
            size_t end_quote = line.find('"', start_quote + 1);
            if (start_quote != std::string::npos && end_quote != std::string::npos) {
                version = line.substr(start_quote + 1, end_quote - start_quote - 1);
                packages.push_back(current_pkg + " (v" + version + ")");
                current_pkg = ""; // Reset current package after adding to list
            }
        }
    }
    
    // Display installed packages
    if (packages.empty()) {
        std::cout << "No packages installed.\n";
    } else {
        std::cout << "Installed packages:\n";
        for (const auto& pkg : packages) {
            std::cout << "  - " << pkg << "\n";
        }
    }
    
    return 0;
} 