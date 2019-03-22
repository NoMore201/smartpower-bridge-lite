#include "hid_pnp.h"

#include <iostream>
#include <vector>
#include <memory>

#include <signal.h>
#include <getopt.h>

const char* program_name = "smartpower";

const struct option long_opts[] = {
    {"help", 0, NULL, 'h'},
    {"enumerate", 0, NULL, 'e'},
    {"all", 0, NULL, 'a'}
};

const char* short_opts = "hea";

std::vector<std::shared_ptr<HID_PnP>> pnp_list;

void signal_handler(int signum) {
    std::cout << "Benchmark ending Signal" << signum << std::endl;
    pnp_list[0]->stop_sampling();
    std::cout.flush();
}

static void print_help() {
    std::cout
        << "Usage:  ./" << program_name << " [options]" << std::endl
        << "  -h  --help       Display this usage informations" << std::endl
        << "  -e  --enumerate  List available power meters connected" << std::endl
        << "  -a  --all        Connect to all available devices" << std::endl;
}

int main(int argc, char *argv[]){
    int next_option = 0;
    device_list lst;

    if (lst.count() == 0) {
        std::cout << "No devices connected" << std::endl;
        exit(EXIT_FAILURE);
    }
    for (auto it = lst.begin(); it != lst.end(); it++) {
        pnp_list.push_back( std::make_shared<HID_PnP>( (*it)->path ) );
    }

    while (next_option != -1) {
        next_option = getopt_long(argc, argv, short_opts, long_opts, NULL);

        switch (next_option) {
            case 'h':
                print_help();
                exit(EXIT_SUCCESS);
                break;
            case 'e':
                lst.print_devices();
                exit(EXIT_SUCCESS);
                break;
            case 'a':
            case -1:
                break;
            default:
                print_help();
                exit(EXIT_FAILURE);
        }
    }

    std::cout << "Autoscript started!" << std::endl;

    /* Signal to terminate measurements */
    signal(SIGUSR1, signal_handler);

    pnp_list[0]->start_sampling();

    return 0;
}
