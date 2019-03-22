#ifndef HID_PNP_H
#define HID_PNP_H

#include <fstream>

#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

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

    void start_sampling();
    void stop_sampling();

    void set_filename(char * const name);

    void toggle_onoff();
    void PollUSB();

private:
    // Device status
    hid_device *device;
    char *device_path;

    bool isConnected;
    bool onOffStatus;
    bool startStopStatus;
    bool toggleStartStop;
    bool toggleOnOff;
    bool quit;

    // Sampling
    int current_timeout;
    bool skip;
    int lastCommand;
    unsigned char buf[MAX_STR];
    unsigned char buf2[MAX_STR];
    int count;

    // Timing
    time_t start_time;
    time_t stop_time;
    double duration;

    // Logging
    std::ofstream log_file;
    std::string file_name;
    char voltage[7],current[7],power[7],energy[7];

    void CloseDevice();

    void save_data(unsigned char* buf);
    char * getDtTm (char *buff);
};

#endif // HID_PNP_H
