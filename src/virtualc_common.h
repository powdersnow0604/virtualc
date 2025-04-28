#pragma once

#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <optional>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <system_error>
#include <cxxopts.hpp>
#include <toml++/toml.hpp>
#include <set>
#include <ctime>

namespace fs = std::filesystem;

// Common constants
extern const char* GITIGNORE_CONTENT;
extern const char* IGNOREPATH_CONTENT;

// Directory where custom library scripts are stored
#ifdef VIRTUALC_BIN_DIR
    extern const char* source_dir;
#else
    // Fallback for when not defined during compilation
    extern const char* source_dir;
#endif

// Library scripts directory
extern std::string libs_dir;

// Utility functions
void create_file(const fs::path& path, const std::string& content = "");
std::string find_gcc_path();
std::string find_gpp_path();
std::set<std::string> read_lines_set(const fs::path& file);
std::string trim(const std::string& s);
std::string run_cmd(const std::string& cmd);
bool is_package_installed(const fs::path& libpath, const std::string& pkg);
void append_libpath(const fs::path& libpath, const std::string& pkg, const std::string& version,
                    const std::vector<std::string>& includes, const std::vector<std::string>& libnames, const std::vector<std::string>& libpaths);
void add_dependency_toml(const fs::path& tomlfile, const std::string& pkg, const std::string& version);
std::string to_uppercase(const std::string& str);
std::string to_lowercase(const std::string& str);
int execute_command(const std::string& command);
bool check_package_installed(const fs::path& libpath, const std::string& pkg);
std::vector<std::string> build_compiler_args(const fs::path& libpath_file);
std::vector<std::string> get_dependencies(const fs::path& toml_file);
std::string get_compiler_path(const fs::path& toml_file);
void remove_dependency_toml(const fs::path& tomlfile, const std::string& pkg);
bool remove_package_from_libpath(const fs::path& libpath_file, const std::string& pkg);

// Create project with the given parameters
void create_project(const fs::path& root, const std::optional<std::string>& compiler, const std::optional<std::string>& global_install);

// Print help message
void print_help(); 