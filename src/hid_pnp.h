#ifndef HID_PNP_H
#define HID_PNP_H

#include <fstream>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>

#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

#include <hidapi/hidapi.h>

#define MAX_STR 65

class device_iterator :
    public std::iterator<std::forward_iterator_tag, struct hid_device_info* > {

        struct hid_device_info *current;

        public:
            explicit device_iterator(struct hid_device_info * dev_list)
                : current(dev_list) {}
            device_iterator(const device_iterator &di)
                : current(di.current) {}

            device_iterator& operator++() {
                current = current->next;
                return *this;
            }

            device_iterator operator++(int) {
                device_iterator tmp(*this);
                ++(*this);
                return tmp;
            }

            bool operator==(const device_iterator &di) {
                return di.current == current;
            }

            bool operator!=(const device_iterator &di) {
                return di.current != current;
            }

            struct hid_device_info * operator*() {
                return current;
            }
};

class device_list {
    public:
        device_list();
        ~device_list();

        device_list(const device_list &d) =delete;
        void operator=(const device_list &d) =delete;

        void print_devices();
        int count();
        device_iterator begin();
        device_iterator end();
    private:
        struct hid_device_info *dev_list;
};

class HID_PnP {
    public:
        explicit HID_PnP(char* device_path);
        ~HID_PnP();

        void run();
        void toggle_sampling();

        void poll();
        void shutdown();

        static void call_handlers(int signum);

    private:
        static std::vector<HID_PnP*> instances;

        // Device status
        hid_device *device;
        char *device_path;

        bool onOffStatus;
        bool startStopStatus;
        bool toggleStartStop;
        bool toggleOnOff;
        bool firstRun;
        std::atomic<bool> saveState;
        std::atomic<bool> quit;

        // Sampling
        std::chrono::milliseconds wait_time;
        unsigned char request[MAX_STR];
        unsigned char response[MAX_STR];
        unsigned int count;

        // Timing
	std::chrono::time_point<std::chrono::system_clock> start_time;
	std::chrono::time_point<std::chrono::system_clock> stop_time;

        // Logging
        std::ofstream log_file;
        std::string file_name;
        std::thread worker_thread;
        char voltage[7],current[7],power[7],energy[7];

        void close_device();
        void toggle_start_stop();
        void toggle_on_off();
        void get_version();
        void get_status();
        void get_data();

        void save_data();
        char * getDtTm (char *buff);
};

#endif // HID_PNP_H
