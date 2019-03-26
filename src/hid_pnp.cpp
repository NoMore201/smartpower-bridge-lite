#include "hid_pnp.h"

#include <iostream>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <chrono>

#include <signal.h>

#define REQUEST_DATA        		0x37
#define REQUEST_STARTSTOP   		0x80
#define REQUEST_STATUS      		0x81
#define REQUEST_ONOFF       		0x82
#define REQUEST_VERSION     		0x83
#define STANDBY_POLLING_TIMEOUT		250
#define MONITOR_POLLING_TIMEOUT		100

#define MANUFACTURER_ID             0x04d8
#define PRODUCT_ID                  0x003f

#define DTTMFMT "%Y-%m-%d %H:%M:%S "
#define DTTMSZ 30

device_list::device_list() {
    dev_list = hid_enumerate(MANUFACTURER_ID, PRODUCT_ID);
}

device_list::~device_list() {
    if (dev_list != NULL) {
        hid_free_enumeration(dev_list);
    }
}

void device_list::print_devices() {
    struct hid_device_info *current_dev;

    current_dev = dev_list;
    if (current_dev == NULL) {
        std::cout << "No device found" << std::endl;
        return;
    }
    while (current_dev) {
        std::wcout << current_dev->product_string << ", "
            << current_dev->manufacturer_string
            << " serial=" << current_dev->path
            << " iface_num=" << current_dev->interface_number
            << std::endl;
        current_dev = current_dev->next;
    }
}

int device_list::count() {
    struct hid_device_info *cur = dev_list;
    int count = 0;
    if (cur == NULL) {
        return count;
    }
    count++;
    while (cur->next != NULL) {
        cur = cur->next;
        count++;
    }
    return count;
}

device_iterator device_list::begin() {
    return device_iterator(dev_list);
}

device_iterator device_list::end() {
    return device_iterator(NULL);
}

HID_PnP::HID_PnP(char* path)
        : device(nullptr)
        , device_path(path)
        , toggleStartStop(0)
        , toggleOnOff(0)
        , firstRun(true)
        , saveState(false)
        , quit(false)
        , wait_time(STANDBY_POLLING_TIMEOUT)
        , count(0)
{
    memset(request, 0x00, MAX_STR);
    memset(response, 0x00, MAX_STR);
    std::ostringstream oss;
    oss << "log-" << path << ".csv";
    file_name = oss.str();
    instances.push_back(this);
}

void HID_PnP::get_version() {
    // reset buffers
    memset(request, 0x00, sizeof(request));
    memset(response, 0x00, sizeof(response));

    request[1] = REQUEST_VERSION;

    if (hid_write(device, request, sizeof(request)) == -1) {
        close_device();
        return;
    }

    if (hid_read(device, response, sizeof(response)) == -1) {
        close_device();
        return;
    }
}

void HID_PnP::get_status() {
    // reset buffers
    memset(request, 0x00, sizeof(request));
    memset(response, 0x00, sizeof(response));

    request[1] = REQUEST_STATUS;

    if (hid_write(device, request, sizeof(request)) == -1) {
        close_device();
        std::runtime_error("Cannot write to device");
    }
    if (hid_read(device, response, sizeof(response)) == -1) {
        close_device();
        std::runtime_error("Cannot read to device");
    }
    /*
    std::cout << "response code: " << (int)response[0]  << std::endl;
    std::cout << "startStopStatus: " << (int)response[1] << std::endl;
    std::cout << "onOffStatus: " << (int)response[2] << std::endl;
    */
}

void HID_PnP::get_data() {
    // reset buffers
    memset(request, 0x00, sizeof(request));
    memset(response, 0x00, sizeof(response));

    request[1] = REQUEST_DATA;

    if (hid_write(device, request, sizeof(request)) == -1) {
        close_device();
        std::runtime_error("Cannot write to device");
    }
    if (hid_read(device, response, sizeof(response)) == -1) {
        close_device();
        std::runtime_error("Cannot read to device");
    }
}

void HID_PnP::toggle_start_stop() {
    memset(request, 0x00, sizeof(request));
    request[1] = REQUEST_STARTSTOP;

    if (hid_write(device, request, sizeof(request)) == -1) {
        close_device();
    }
}

void HID_PnP::toggle_on_off() {
    memset(request, 0x00, sizeof(request));
    request[1] = REQUEST_ONOFF;

    if (hid_write(device, request, sizeof(request)) == -1) {
        close_device();
    }
}


HID_PnP::~HID_PnP() {
    instances.erase(
            std::remove(instances.begin(), instances.end(), this),
            instances.end());
}

void HID_PnP::start_sampling() {
   // Create Log file
   std::cout << "File name:" << file_name << std::endl;
   log_file.open(file_name.c_str(), std::ios::out | std::ios::app);

   // Start sampling
   toggleStartStop = true;
   std::cout << "Start sampling device " << device_path << std::endl;
   while(!quit) {
      poll();
      std::this_thread::sleep_for(wait_time);
   }
   close_device();
   log_file.close();
}

std::vector<HID_PnP*> HID_PnP::instances;

void HID_PnP::call_handlers(int signum) {
    switch (signum) {
        case SIGUSR1:
            {
                for (auto i : instances) {
                    i->stop_sampling();
                }
            }
            break;
        case SIGUSR2:
            {
                for (auto i : instances) {
                    i->toggle_device_power();
                }
            }
            break;
        case SIGTERM:
            {
                for (auto i : instances) {
                    i->shutdown();
                }
            }
            break;
        default:
            break;
    }
}

void HID_PnP::stop_sampling() {
    toggleStartStop = true;
    if (onOffStatus && startStopStatus) {
        saveState = true;
    }
}

void HID_PnP::shutdown() {
    quit = true;
}

void HID_PnP::poll()
{
    if (!device) {
        //Connecting device

        device = hid_open_path(device_path);

        if (!device) {
            throw std::runtime_error("Unable to open HID device");
        }

        hid_set_nonblocking(device, true);

        get_version();
    } else {
        if (toggleStartStop) {
            toggleStartStop = false;
            toggle_start_stop();
            return;
        }

        if (toggleOnOff) {
            toggleOnOff = false;
            toggle_on_off();
            return;
        }

        if (response[0] == REQUEST_VERSION) {
            get_status();
            count = 0;
        } else if (response[0] == REQUEST_DATA) {
            if (saveState) {
                save_data();
                saveState = false;
            }
            get_status();
        } else if (response[0] == REQUEST_STATUS) {
            startStopStatus = response[1];
            onOffStatus = response[2];
            if (firstRun && !startStopStatus) {
                // if device was still measuring before starting this
                // program, it may be stuck in a stopped state. To avoid this
                // situation we toggle it again
                firstRun = false;
                toggleStartStop = true;
                return;
            }
            if (count == 9) {
                get_status();
            } else {
                get_data();
            }
            count = 0;
        } else {
            // if previous response code is unrecognized
            // we force it to get the status again
            get_status();
        }
        count++;
    }
}

void HID_PnP::save_data() {
    // Showing values
    char buff[DTTMSZ];

    strncpy(voltage, (char*)&response[2], 6);
    //std::cout << "Voltage: " << voltage << "V, ";
    memset(current, '\0', 7);
    strncpy(current, (char*)&response[10], 5);
    //std::cout << "Current: " << current << "A, ";
    memset(power, '\0', 7);
    strncpy(power, (char*)&response[18], 5);
    //std::cout << "Power: " << power << "W, ";
    memset(energy, '\0', 7);
    strncpy(energy, (char*)&response[26], 5);
    std::cout << "Energy: " << energy << "Wh" << std::endl;

    std::ostringstream oss;
    oss << "[" << getDtTm(buff) << "] " << energy << std::endl;
    std::string log_string = oss.str();
    log_file << log_string;
}

void HID_PnP::toggle_device_power() {
    toggleOnOff = true;
}

void HID_PnP::close_device() {
    hid_close(device);
    device = NULL;
    toggleOnOff = 0;
    toggleStartStop = 0;
    quit = true;
}

char* HID_PnP::getDtTm (char *buff) {
    std::time_t t = std::time(nullptr);
    std::strftime(buff, DTTMSZ, DTTMFMT, std::localtime(&t));
    return buff;
}

