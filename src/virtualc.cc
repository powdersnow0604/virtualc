#include "virtualc_common.h"
#include "virtualc_init.h"
#include "virtualc_install.h"
#include "virtualc_uninstall.h"
#include "virtualc_list.h"
#include "virtualc_run.h"
#include "virtualc_upgrade.h"
#include "virtualc_clear.h"

int main(int argc, char** argv) {
    try {
        if (argc < 2) {
            print_help();
            return 1;
        }

        std::string command = argv[1];

        if (command == "init") {
            // Adjust argc/argv to omit the subcommand
            return init_main(argc - 1, argv + 1);
        } else if (command == "install") {
            if (argc < 3) {
                std::cerr << "Error: No package specified for installation" << std::endl;
                return 1;
            }
            // Collect all package names from arguments
            std::vector<std::string> packages;
            for (int i = 2; i < argc; i++) {
                packages.push_back(argv[i]);
            }
            return install_main(packages);
        } else if (command == "uninstall") {
            if (argc < 3) {
                std::cerr << "Error: No package specified for uninstallation" << std::endl;
                return 1;
            }
            // Collect all package names from arguments
            std::vector<std::string> packages;
            for (int i = 2; i < argc; i++) {
                packages.push_back(argv[i]);
            }
            return uninstall_main(packages);
        } else if (command == "list") {
            return list_packages_main();
        } else if (command == "run") {
            if (argc < 3) {
                std::cerr << "Error: No filename specified to run" << std::endl;
                return 1;
            }
            return run_main(argc - 2, argv + 2);
        } else if (command == "upgrade") {
            return upgrade_libs_main();
        } else if (command == "clear") {
            return clear_main();
        } else if (command == "--help" || command == "-h") {
            print_help();
            return 0;
        } else {
            std::cerr << "Unknown command: " << command << std::endl;
            return 1;
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
} 