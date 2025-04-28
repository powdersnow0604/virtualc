#include "virtualc_common.h"

const char* GITIGNORE_CONTENT = R"(# Build artifacts
*.o
*.obj
*.lo
*.a
*.lib
*.so
*.so.*
*.dll
*.dylib
*.exe
*.out
*.app

# Dependency files
*.d

# Generated files
*.log
*.tmp

# CMake
CMakeFiles/
CMakeCache.txt
cmake_install.cmake
Makefile
build/
out/

# Visual Studio
*.vcxproj*
*.sln
*.user
*.opensdf
*.sdf
*.pdb
*.idb
*.ipch
*.aps
*.ncb

# CodeBlocks
*.cbp
*.layout
*.depend

# Eclipse
.project
.cproject
.settings/

# Other IDEs
*.idea/
*.vscode/

# OS files
.DS_Store
Thumbs.db

# virtualc
.libpath
.verified
.venv
)";

const char* IGNOREPATH_CONTENT = R"(/usr/local/lib
/usr/local/include
)";

// Directory where custom library scripts are stored
#ifdef VIRTUALC_BIN_DIR
    const char* source_dir = VIRTUALC_BIN_DIR;
#else
    // Fallback for when not defined during compilation
    const char* source_dir = "/usr/local/bin/virtualcdir";
#endif

// Define the directory where custom library scripts are stored
std::string libs_dir = std::string(source_dir) + "/libs";

void create_file(const fs::path& path, const std::string& content) {
    std::ofstream ofs(path);
    if (!ofs) {
        throw std::runtime_error("Failed to create file: " + path.string());
    }
    ofs << content;
}

// Function to find the path to GCC using 'which' command
std::string find_gcc_path() {
    std::string gcc_path = "";
    FILE* pipe = popen("which gcc 2>/dev/null", "r");
    if (!pipe) return gcc_path;
    
    char buffer[256];
    if (fgets(buffer, sizeof(buffer), pipe)) {
        gcc_path = std::string(buffer);
        // Remove trailing newline if present
        if (!gcc_path.empty() && gcc_path.back() == '\n') {
            gcc_path.pop_back();
        }
    }
    pclose(pipe);
    
    return gcc_path;
}

std::string find_gpp_path() {
    std::string gpp_path = "";
    FILE* pipe = popen("which g++ 2>/dev/null", "r");
    if (!pipe) return gpp_path;
    
    char buffer[256];
    if (fgets(buffer, sizeof(buffer), pipe)) {
        gpp_path = std::string(buffer);
        // Remove trailing newline if present
        if (!gpp_path.empty() && gpp_path.back() == '\n') {
            gpp_path.pop_back();
        }
    }
    pclose(pipe);
    
    return gpp_path;
}

void create_project(const fs::path& root, const std::optional<std::string>& compiler, const std::optional<std::string>& global_install) {
    // Create directories
    fs::create_directories(root / ".venv");

    // Create files
    create_file(root / ".gitignore", GITIGNORE_CONTENT);
    create_file(root / ".ignorepath", IGNOREPATH_CONTENT);
    create_file(root / ".libpath", ""); // Empty at init
    create_file(root / "README.md", ""); // Empty or default message

    // cproject.toml
    toml::table tbl;
    // Ensure the [project] table exists and get a reference to it
    tbl.insert_or_assign("project", toml::table{});
    auto& proj = *tbl.get_as<toml::table>("project");
    if (compiler) {
        proj.insert_or_assign("compilerpath", *compiler);
    } else {
        proj.insert_or_assign("compilerpath", "");
    }
    
    if (global_install) {
        proj.insert_or_assign("installpath", *global_install);
    }
    proj.insert_or_assign("dependencies", toml::array{}); // Empty at init

    std::ofstream tomlfile(root / "cproject.toml");
    if (!tomlfile) throw std::runtime_error("Failed to create cproject.toml");
    tomlfile << tbl;
    tomlfile.close();
}

// Utility: read lines from a file into a set
std::set<std::string> read_lines_set(const fs::path& file) {
    std::set<std::string> lines;
    std::ifstream in(file);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty()) lines.insert(line);
    }
    return lines;
}

// Utility: trim whitespace
std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    size_t end = s.find_last_not_of(" \t\n\r");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

// Utility: run a shell command and get output
std::string run_cmd(const std::string& cmd) {
    std::string result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return result;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe)) result += buffer;
    pclose(pipe);
    return result;
}

// Utility: check if a package is in .libpath
bool is_package_installed(const fs::path& libpath, const std::string& pkg) {
    std::ifstream in(libpath);
    std::string line;
    while (std::getline(in, line)) {
        if (line.find("[" + pkg + "]") != std::string::npos) return true;
    }
    return false;
}

// Utility: append package info to .libpath
void append_libpath(const fs::path& libpath, const std::string& pkg, const std::string& version,
                  const std::vector<std::string>& includes, const std::vector<std::string>& libnames, const std::vector<std::string>& libpaths) {
    std::ofstream out(libpath, std::ios::app);
    out << "[" << pkg << "]\n";
    out << "version = \"" << version << "\"\n";
    out << "includes = [";
    for (size_t i = 0; i < includes.size(); ++i) {
        if (i) out << ", ";
        out << "\"" << includes[i] << "\"";
    }
    out << "]\nlibnames = [";
    for (size_t i = 0; i < libnames.size(); ++i) {
        if (i) out << ", ";
        out << "\"" << libnames[i] << "\"";
    }
    out << "]\nlibpaths = [";
    for (size_t i = 0; i < libpaths.size(); ++i) {
        if (i) out << ", ";
        out << "\"" << libpaths[i] << "\"";
    }
    out << "]\n\n";
}

// Utility: update dependencies in cproject.toml
void add_dependency_toml(const fs::path& tomlfile, const std::string& pkg, const std::string& version) {
    auto tbl = toml::parse_file(tomlfile.string());
    auto* proj = tbl.get_as<toml::table>("project");
    if (!proj) return;
    toml::array* arr = nullptr;
    if (auto dep_node = proj->get("dependencies"); dep_node && dep_node->is_array()) {
        arr = dep_node->as_array();
    } else {
        arr = proj->insert_or_assign("dependencies", toml::array{}).first->second.as_array();
    }
    std::string depstr = pkg + "==" + version;
    for (auto&& v : *arr) {
        if (v.value<std::string>().value_or("") == depstr) return;
    }
    arr->push_back(depstr);
    std::ofstream out(tomlfile);
    out << tbl;
}

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

// Function to check if the package is installed via .libpath
bool check_package_installed(const fs::path& libpath, const std::string& pkg) {
    return is_package_installed(libpath, pkg);
}

// Function to build compiler arguments from .libpath
std::vector<std::string> build_compiler_args(const fs::path& libpath_file) {
    std::vector<std::string> args;
    
    std::ifstream file(libpath_file);
    if (!file) {
        return args;
    }
    
    std::string line;
    std::string current_package;
    std::vector<std::string> includes;
    std::vector<std::string> libnames;
    std::vector<std::string> libpaths;
    
    // Parse .libpath file
    while (std::getline(file, line)) {
        line = trim(line);
        
        if (line.empty()) continue;
        
        // Process section header [package-name]
        if (line[0] == '[') {
            size_t end_bracket = line.find(']');
            if (end_bracket != std::string::npos) {
                // Start a new package
                current_package = line.substr(1, end_bracket - 1);
            }
            continue;
        }
        
        // Process package contents
        if (!current_package.empty()) {
            if (line.find("includes = [") == 0) {
                // Extract includes array
                size_t start_array = line.find('[');
                size_t end_array = line.find(']');
                if (start_array != std::string::npos && end_array != std::string::npos) {
                    std::string array_content = line.substr(start_array + 1, end_array - start_array - 1);
                    // Parse the array elements (simple split by commas)
                    size_t pos = 0;
                    while ((pos = array_content.find(',')) != std::string::npos) {
                        std::string path = array_content.substr(0, pos);
                        path = trim(path);
                        // Remove quotes if present
                        if (path.size() >= 2 && path.front() == '"' && path.back() == '"') {
                            path = path.substr(1, path.size() - 2);
                        }
                        if (!path.empty()) {
                            includes.push_back(path);
                        }
                        array_content.erase(0, pos + 1);
                    }
                    // Process the last element
                    std::string path = trim(array_content);
                    if (path.size() >= 2 && path.front() == '"' && path.back() == '"') {
                        path = path.substr(1, path.size() - 2);
                    }
                    if (!path.empty()) {
                        includes.push_back(path);
                    }
                }
            }
            else if (line.find("libnames = [") == 0) {
                // Extract libnames array
                size_t start_array = line.find('[');
                size_t end_array = line.find(']');
                if (start_array != std::string::npos && end_array != std::string::npos) {
                    std::string array_content = line.substr(start_array + 1, end_array - start_array - 1);
                    // Parse the array elements
                    size_t pos = 0;
                    while ((pos = array_content.find(',')) != std::string::npos) {
                        std::string lib = array_content.substr(0, pos);
                        lib = trim(lib);
                        // Remove quotes if present
                        if (lib.size() >= 2 && lib.front() == '"' && lib.back() == '"') {
                            lib = lib.substr(1, lib.size() - 2);
                        }
                        if (!lib.empty()) {
                            libnames.push_back(lib);
                        }
                        array_content.erase(0, pos + 1);
                    }
                    // Process the last element
                    std::string lib = trim(array_content);
                    if (lib.size() >= 2 && lib.front() == '"' && lib.back() == '"') {
                        lib = lib.substr(1, lib.size() - 2);
                    }
                    if (!lib.empty()) {
                        libnames.push_back(lib);
                    }
                }
            }
            else if (line.find("libpaths = [") == 0) {
                // Extract libpaths array
                size_t start_array = line.find('[');
                size_t end_array = line.find(']');
                if (start_array != std::string::npos && end_array != std::string::npos) {
                    std::string array_content = line.substr(start_array + 1, end_array - start_array - 1);
                    // Parse the array elements
                    size_t pos = 0;
                    while ((pos = array_content.find(',')) != std::string::npos) {
                        std::string path = array_content.substr(0, pos);
                        path = trim(path);
                        // Remove quotes if present
                        if (path.size() >= 2 && path.front() == '"' && path.back() == '"') {
                            path = path.substr(1, path.size() - 2);
                        }
                        if (!path.empty()) {
                            libpaths.push_back(path);
                        }
                        array_content.erase(0, pos + 1);
                    }
                    // Process the last element
                    std::string path = trim(array_content);
                    if (path.size() >= 2 && path.front() == '"' && path.back() == '"') {
                        path = path.substr(1, path.size() - 2);
                    }
                    if (!path.empty()) {
                        libpaths.push_back(path);
                    }
                }
            }
        }
    }
    
    // Build args from parsed information
    for (const auto& include : includes) {
        args.push_back("-I" + include);
    }
    
    for (const auto& libpath : libpaths) {
        args.push_back("-L" + libpath);
    }
    
    for (const auto& lib : libnames) {
        args.push_back("-l" + lib);
    }
    
    return args;
}

// Get dependencies from cproject.toml
std::vector<std::string> get_dependencies(const fs::path& toml_file) {
    std::vector<std::string> deps;
    
    try {
        auto tbl = toml::parse_file(toml_file.string());
        auto* proj = tbl.get_as<toml::table>("project");
        if (!proj) return deps;
        
        if (auto dep_node = proj->get("dependencies"); dep_node && dep_node->is_array()) {
            auto* deps_arr = dep_node->as_array();
            if (!deps_arr) return deps;
            
            for (auto& dep : *deps_arr) {
                if (auto dep_str = dep.value<std::string>()) {
                    std::string dep_val = *dep_str;
                    // Extract package name from "pkg==version"
                    size_t pos = dep_val.find("==");
                    if (pos != std::string::npos) {
                        deps.push_back(dep_val.substr(0, pos));
                    } else {
                        deps.push_back(dep_val);
                    }
                }
            }
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error parsing cproject.toml: " << ex.what() << std::endl;
    }
    
    return deps;
}

// Get compiler path from cproject.toml
std::string get_compiler_path(const fs::path& toml_file) {
    try {
        auto tbl = toml::parse_file(toml_file.string());
        auto* proj = tbl.get_as<toml::table>("project");
        if (!proj) return "";
        
        if (auto compiler_node = proj->get("compilerpath"); compiler_node && compiler_node->is_string()) {
            std::string compiler = compiler_node->value_or("");
            if (!compiler.empty()) {
                return compiler;
            }
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error parsing cproject.toml: " << ex.what() << std::endl;
    }
    
    // Default to 'gcc' if not found
    return "gcc";
}

// Function to remove package info from .libpath
bool remove_package_from_libpath(const fs::path& libpath_file, const std::string& pkg) {
    std::ifstream infile(libpath_file);
    if (!infile) {
        return false;
    }
    
    std::string line;
    std::vector<std::string> lines;
    bool in_package_section = false;
    bool found = false;
    
    // Read all lines, skipping the target package's section
    while (std::getline(infile, line)) {
        // Check if this line starts a new package section
        if (!line.empty() && line[0] == '[') {
            std::string section_pkg = line.substr(1, line.find(']') - 1);
            if (section_pkg == pkg) {
                in_package_section = true;
                found = true;
                continue; // Skip this line and package section
            } else {
                in_package_section = false;
            }
        }
        
        // Keep lines not in the target package section
        if (!in_package_section) {
            lines.push_back(line);
        }
    }
    
    infile.close();
    
    // Write back the file without the package section
    std::ofstream outfile(libpath_file);
    if (!outfile) {
        return false;
    }
    
    for (const auto& l : lines) {
        outfile << l << std::endl;
    }
    
    return found;
}

// Function to remove package from cproject.toml dependencies
void remove_dependency_toml(const fs::path& tomlfile, const std::string& pkg) {
    try {
        auto tbl = toml::parse_file(tomlfile.string());
        auto* proj = tbl.get_as<toml::table>("project");
        if (!proj) return;
        
        if (auto dep_node = proj->get("dependencies"); dep_node && dep_node->is_array()) {
            auto* deps_arr = dep_node->as_array();
            if (!deps_arr) return;
            
            // Create a new array without the target package
            toml::array new_deps;
            
            for (auto& dep : *deps_arr) {
                if (auto dep_str = dep.value<std::string>()) {
                    std::string dep_val = *dep_str;
                    // Check if dependency starts with pkg name followed by ==
                    if (dep_val.substr(0, pkg.length()) != pkg || 
                        (dep_val.length() > pkg.length() && dep_val.substr(pkg.length(), 2) != "==")) {
                        new_deps.push_back(dep_val);
                    }
                }
            }
            
            // Replace the dependencies array
            proj->insert_or_assign("dependencies", std::move(new_deps));
            
            // Write updated TOML to file
            std::ofstream out(tomlfile);
            out << tbl;
        }
    } catch (const std::exception& ex) {
        std::cerr << "Warning: Error removing dependency from cproject.toml: " << ex.what() << std::endl;
    }
}

void print_help() {
    std::cerr << "Usage: vc <command> [arguments]" << std::endl;
    std::cerr << "Commands:" << std::endl;
    std::cerr << "  init [path] [options]  Initialize a new project" << std::endl;
    std::cerr << "  install <packages...>  Install one or more packages" << std::endl;
    std::cerr << "  uninstall <packages...> Uninstall one or more packages" << std::endl;
    std::cerr << "  list                   List installed packages" << std::endl;
    std::cerr << "  run <filename>         Compile and run a file with dependencies" << std::endl;
    std::cerr << "  upgrade               Upgrade library scripts from repository" << std::endl;
    std::cerr << "  clear                  Remove all project files and directories" << std::endl;
    std::cerr << "Options for init:" << std::endl;
    std::cerr << "  -c, --compiler         Set compiler path (can be any compiler)" << std::endl;
    std::cerr << "  -x, --cxx              Use g++ as default compiler instead of gcc" << std::endl;
    std::cerr << "  -g, --global           Install packages globally at specified path" << std::endl;
    std::cerr << "  -h, --help             Print this help message" << std::endl;
} 