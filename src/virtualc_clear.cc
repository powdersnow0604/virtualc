#include "virtualc_clear.h"

// Implement clear subcommand
int clear_main() {
    fs::path cwd = fs::current_path();
    
    // List of files and directories to remove
    std::vector<fs::path> to_remove = {
        cwd / ".venv",
        cwd / ".gitignore",
        cwd / ".ignorepath",
        cwd / ".libpath",
        cwd / ".verified",
        cwd / "cproject.toml",
        cwd / "README.md"
    };
    
    bool all_removed = true;
    
    std::cout << "Clearing project files..." << std::endl;
    
    for (const auto& path : to_remove) {
        if (fs::exists(path)) {
            try {
                if (fs::is_directory(path)) {
                    std::cout << "Removing directory: " << path << std::endl;
                    fs::remove_all(path);
                } else {
                    std::cout << "Removing file: " << path << std::endl;
                    fs::remove(path);
                }
            } catch (const std::exception& ex) {
                std::cerr << "Error removing " << path << ": " << ex.what() << std::endl;
                all_removed = false;
            }
        }
    }
    
    if (all_removed) {
        std::cout << "Project cleared successfully." << std::endl;
        return 0;
    } else {
        std::cerr << "Some project files could not be removed." << std::endl;
        return 1;
    }
} 