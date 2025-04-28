cmake_minimum_required(VERSION 3.10)
project(virtualc_init CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define virtualcdir path
set(VIRTUALC_BIN_DIR "/usr/local/bin/virtualcdir")
add_definitions(-DVIRTUALC_BIN_DIR="${VIRTUALC_BIN_DIR}")

include(FetchContent)

# Fetch toml++
FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG        v3.4.0 # You can update to a newer tag if needed
)
FetchContent_MakeAvailable(tomlplusplus)

# Fetch cxxopts
FetchContent_Declare(
    cxxopts
    GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
    GIT_TAG        v3.1.1 # You can update to a newer tag if needed
)
FetchContent_MakeAvailable(cxxopts)

# List all source files
set(SOURCES
    src/virtualc.cc
    src/virtualc_common.cc
    src/virtualc_init.cc
    src/virtualc_install.cc
    src/virtualc_uninstall.cc
    src/virtualc_list.cc
    src/virtualc_run.cc
    src/virtualc_upgrade.cc
    src/virtualc_clear.cc
)

add_executable(vc ${SOURCES})

# Link toml++ and cxxopts
# tomlplusplus is header-only, so just add include dir
# cxxopts is also header-only
target_include_directories(vc PRIVATE 
    ${tomlplusplus_SOURCE_DIR}/include
    ${cxxopts_SOURCE_DIR}/include
)

# Add installation targets
install(TARGETS vc
    RUNTIME DESTINATION bin
)

# Add custom install command to clone the linux_scripts repository
# and place the 'libs' directory in the VIRTUALC_BIN_DIR
install(CODE "
    message(STATUS \"Cloning linux_scripts repository...\")
    # Define temp directory path
    set(TEMP_DIR \"/tmp/vc_install_temp\")
    
    # Remove temp directory if it already exists
    if(EXISTS \"\${TEMP_DIR}\")
        message(STATUS \"Cleaning up existing temporary directory: \${TEMP_DIR}\")
        execute_process(
            COMMAND rm -rf \"\${TEMP_DIR}\"
            RESULT_VARIABLE exit_code
        )
        if(NOT exit_code EQUAL 0)
            message(WARNING \"Failed to remove existing temporary directory. Will try to continue anyway.\")
        endif()
    endif()
    
    # Create temp directory
    execute_process(
        COMMAND mkdir -p \"\${TEMP_DIR}\"
        RESULT_VARIABLE exit_code
    )
    if(NOT exit_code EQUAL 0)
        message(FATAL_ERROR \"Failed to create temporary directory\")
    endif()
    
    # Clone repository
    execute_process(
        COMMAND git clone --depth 1 https://github.com/powdersnow0604/linux_scripts.git \"\${TEMP_DIR}\"
        RESULT_VARIABLE exit_code
    )
    if(NOT exit_code EQUAL 0)
        message(FATAL_ERROR \"Failed to clone repository\")
    endif()
    
    # Check if libs directory exists in the cloned repository
    if(NOT EXISTS \"\${TEMP_DIR}/libs\")
        message(FATAL_ERROR \"The 'libs' directory was not found in the cloned repository.\")
    endif()
    
    # Check if authorize.sh exists
    if(EXISTS \"\${TEMP_DIR}/authorize.sh\")
        message(STATUS \"Running authorize.sh script to authorize all shell scripts\")
        # Make the script executable
        execute_process(
            COMMAND chmod +x \"\${TEMP_DIR}/authorize.sh\"
            RESULT_VARIABLE exit_code
        )
        if(NOT exit_code EQUAL 0)
            message(WARNING \"Failed to make authorize.sh executable. Authorization may be incomplete.\")
        endif()
        
        # Execute the authorization script
        execute_process(
            COMMAND sudo \"\${TEMP_DIR}/authorize.sh\"
            WORKING_DIRECTORY \"\${TEMP_DIR}\"
            RESULT_VARIABLE exit_code
        )
        if(NOT exit_code EQUAL 0)
            message(WARNING \"authorize.sh script failed with exit code \${exit_code}. Some scripts may not be properly authorized.\")
        else()
            message(STATUS \"Successfully authorized all shell scripts.\")
        endif()
    else()
        message(WARNING \"authorize.sh not found in the repository. Scripts may need manual authorization.\")
    endif()
    
    # Create libs directory in VIRTUALC_BIN_DIR
    execute_process(
        COMMAND mkdir -p \"${VIRTUALC_BIN_DIR}/libs\"
        RESULT_VARIABLE exit_code
    )
    if(NOT exit_code EQUAL 0)
        message(FATAL_ERROR \"Failed to create libs directory in ${VIRTUALC_BIN_DIR}\")
    endif()
    
    # Copy libs directory
    execute_process(
        COMMAND sudo cp -r \"\${TEMP_DIR}/libs/.\" \"${VIRTUALC_BIN_DIR}/libs/\"
        RESULT_VARIABLE exit_code
    )
    if(NOT exit_code EQUAL 0)
        message(FATAL_ERROR \"Failed to copy libs directory to ${VIRTUALC_BIN_DIR}/libs\")
    endif()
    
    # Clean up
    execute_process(
        COMMAND sudo rm -rf \"\${TEMP_DIR}\"
        RESULT_VARIABLE exit_code
    )
    if(NOT exit_code EQUAL 0)
        message(WARNING \"Failed to clean up temporary directory\")
    else()
        message(STATUS \"Temporary directory cleaned up successfully.\")
    endif()
    
    message(STATUS \"Successfully installed library scripts to ${VIRTUALC_BIN_DIR}/libs\")
")

# If you need to link any libraries, add them here:
# target_link_libraries(virtualc ...) 