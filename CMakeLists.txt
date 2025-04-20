cmake_minimum_required(VERSION 3.14)
project(VirtualC VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Always use FetchContent because i want to use nlohmann_json under directory json not nlohmann
include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(nlohmann_json)

# Define the source files
set(SOURCE_DIR src)
set(SOURCES_VIRTUALC ${SOURCE_DIR}/virtualc.cc)
set(SOURCES_ICL ${SOURCE_DIR}/icl.cc)
set(SOURCES_GCC ${SOURCE_DIR}/gcc.cc)
set(SOURCES_GXX ${SOURCE_DIR}/g++.cc)

# Define installation paths
set(VIRTUALC_BIN_DIR ${CMAKE_INSTALL_PREFIX}/bin/virtualcdir)

# Add definitions for installation paths
add_definitions(-DVIRTUALC_BIN_DIR="${VIRTUALC_BIN_DIR}")

# Add custom command to create the virtualcdir/bin directory during build
# add_custom_command(
#     OUTPUT ${CMAKE_BINARY_DIR}/virtualcdir_bin_dir
#     COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/virtualcdir_bin_dir
#     COMMENT "Creating virtualcdir bin directory"
# )

# Add the virtualc executable
add_executable(virtualc ${SOURCES_VIRTUALC})

# Define the installation directory as a preprocessor macro
target_compile_definitions(virtualc PRIVATE 
    VIRTUALC_BIN_DIR="${VIRTUALC_BIN_DIR}"
)

# Modify virtualc.cc to copy files from VIRTUALC_BIN_DIR to env_name/bin
# target_compile_definitions(virtualc PRIVATE
#     VIRTUALC_DATA_DIR="${VIRTUALC_BIN_DIR}"
# )

# Add the compiler wrapper executables
add_executable(gcc_wrapper ${SOURCES_GCC})
target_link_libraries(gcc_wrapper PRIVATE nlohmann_json)
set_target_properties(gcc_wrapper PROPERTIES OUTPUT_NAME gcc)

add_executable(gxx_wrapper ${SOURCES_GXX})
target_link_libraries(gxx_wrapper PRIVATE nlohmann_json)
set_target_properties(gxx_wrapper PROPERTIES OUTPUT_NAME g++)

add_executable(icl ${SOURCES_ICL})
target_link_libraries(icl PRIVATE nlohmann_json)

# Add installation targets
install(TARGETS virtualc
    RUNTIME DESTINATION bin
)

install(TARGETS gcc_wrapper gxx_wrapper icl
    RUNTIME DESTINATION bin/virtualcdir
)

# Add custom install command to clone the linux_scripts repository
# and place the 'libs' directory in the VIRTUALC_BIN_DIR
install(CODE "
    message(STATUS \"Cloning linux_scripts repository...\")
    
    # Define temp directory path
    set(TEMP_DIR \"/tmp/virtualc_install_temp\")
    
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
        # Change to the directory containing the script
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