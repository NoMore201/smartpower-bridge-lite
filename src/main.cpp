#include "hid_pnp.h"

#include <iostream>

#include <signal.h>

HID_PnP * plugNPlay;

void signal_handler(int signum){
    std::cout << "Benchmark ending Signal" << signum << std::endl;
    plugNPlay->stop_sampling();
    std::cout.flush();
}

int main(int argc, char *argv[]){
    std::cout << "Autoscript started!" << std::endl;

    /* Signal to terminate measurements */
    signal(SIGUSR1, signal_handler);
    
    plugNPlay = new HID_PnP();
    if(argc == 2) {
        plugNPlay->set_filename(argv[1]);
    }
    plugNPlay->toggle_startstop();

    return 0;
}
