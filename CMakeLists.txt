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