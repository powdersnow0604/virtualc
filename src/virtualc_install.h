#pragma once

#include "virtualc_common.h"
#include <vector>

// Install multiple packages
int install_main(const std::vector<std::string>& packages);

// Function to try installing a library using custom script
bool try_install_custom_library(const std::string& lib_name, const std::string& install_path); 