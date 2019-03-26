#include "hid_pnp.h"

#include <iostream>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>

#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

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
        : device(NULL)
        , device_path(path)
        , isConnected(false)
        , toggleStartStop(0)
        , toggleOnOff(0)
        , quit(false)
        , current_timeout(STANDBY_POLLING_TIMEOUT)
{
    memset((void*)&buf[2], 0x00, sizeof(buf) - 2);
    std::ostringstream oss;
    oss << "log-" << path << ".csv";
    file_name = oss.str();
    instances.push_back(this);
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
   std::cout << "Start sampling" << std::endl;
   time(&start_time);
   while(!quit){
      PollUSB();
      usleep(current_timeout);
   }
   toggleStartStop = true;
   PollUSB();
   CloseDevice();
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
                    i->toggle_onoff();
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

void HID_PnP::set_filename(char * const name) {
    file_name = std::string(name);
}

void HID_PnP::stop_sampling() {
    toggleStartStop = true;
    time(&stop_time);
    duration = difftime(stop_time, start_time);
    save_data();
    std::cout << "Stopped sampling" << std::endl;
}

void HID_PnP::shutdown() {
    quit = true;
}

// Done every POLLING_TIMEOUT seconds
void HID_PnP::PollUSB()
{
    buf[0] = 0x00;

    if (isConnected == false) { //Connecting device

        device = hid_open_path(device_path);

        if (!device) {
            throw std::runtime_error("Unable to open HID device");
        }

        if (device) {
            memset((void*)&buf[2], 0x00, sizeof(buf) - 2);
            isConnected = true;
            hid_set_nonblocking(device, true);

            buf[1] = REQUEST_VERSION;

            lastCommand = buf[1];

            if (hid_write(device, buf, sizeof(buf)) == -1) {
                CloseDevice();
                return;
            }

            if (hid_read(device, buf, sizeof(buf)) == -1) {
                CloseDevice();
                return;
            }
        }
    } else {
        if (toggleStartStop == true) {
            toggleStartStop = false;

            unsigned char cmd[MAX_STR] = {0x00,};
            cmd[1] = REQUEST_STARTSTOP;

            if (hid_write(device, cmd, sizeof(cmd)) == -1) {
                CloseDevice();
                return;
            }
        }

        if (toggleOnOff == true) {
            toggleOnOff = false;

            unsigned char cmd[MAX_STR] = {0x00,};
            cmd[1] = REQUEST_ONOFF;

            if (hid_write(device, cmd, sizeof(cmd)) == -1) {
                CloseDevice();
                return;
            }
        }

        lastCommand = buf[1];

        if (!skip) {
            if (hid_write(device, buf, sizeof(buf)) == -1) {
                CloseDevice();
                return;
            }
        }

#ifdef __linux__
        usleep(10);
#else
        _sleep(10);
#endif

        if (hid_read(device, buf, sizeof(buf)) == -1) {
            CloseDevice();
            return;
        }

        if (lastCommand != buf[0]) {
            skip = true;
        } else {
            if (buf[0] == REQUEST_VERSION) {
                buf[1] = REQUEST_STATUS;
                skip = false;
                current_timeout = MONITOR_POLLING_TIMEOUT;
                count = 0;
                memset(buf2, 0x00, MAX_STR);
            } else if (buf[0] == REQUEST_DATA) {
                buf[1] = REQUEST_STATUS;
                memcpy(buf2, buf, MAX_STR);
            } else if (buf[0] == REQUEST_STATUS) {
                startStopStatus = (buf[1] == 0x01);
                onOffStatus = (buf[2] == 0x01);
                if (count == 9)
                    buf[1] = REQUEST_STATUS;
                else
                    buf[1] = REQUEST_DATA;
                count = 0;
            } else {
                if (lastCommand == REQUEST_STATUS)
                    buf[1] = REQUEST_DATA;
                else
                    buf[1] = REQUEST_STATUS;
            }
            skip = false;
        }


        count++;
    }
}

void HID_PnP::save_data() {
    // Showing values
    char buff[DTTMSZ];

    strncpy(voltage, (char*)&buf2[2], 6);
    //std::cout << "Voltage: " << voltage << "V, ";
    memset(current, '\0', 7);
    strncpy(current, (char*)&buf2[10], 5);
    //std::cout << "Current: " << current << "A, ";
    memset(power, '\0', 7);
    strncpy(power, (char*)&buf2[18], 5);
    //std::cout << "Power: " << power << "W, ";
    memset(energy, '\0', 7);
    strncpy(energy, (char*)&buf2[26], 5);
    std::cout << "Energy: " << energy << "Wh, ";

    std::cout << "Duration: " << duration << "s" << std::endl;

    std::ostringstream oss;
    oss << getDtTm(buff) << "," << energy << "," << duration << std::endl;
    std::string log_string = oss.str();
    //std::cout << log_string;
    log_file << log_string;
}

void HID_PnP::toggle_onoff() {
    toggleOnOff = true;
}

void HID_PnP::CloseDevice() {
    hid_close(device);
    device = NULL;
    isConnected = false;
    toggleOnOff = 0;
    toggleStartStop = 0;
}

char *HID_PnP::getDtTm (char *buff) {
    time_t t = time (0);
    strftime (buff, DTTMSZ, DTTMFMT, localtime (&t));
    return buff;
}

