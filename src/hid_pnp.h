#ifndef HID_PNP_H
#define HID_PNP_H

#include <fstream>

#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <hidapi/hidapi.h>

#define MAX_STR 65

class HID_PnP {
public:
    explicit HID_PnP();
    ~HID_PnP();

    void stop_sampling();

    void set_filename(char * const name);

    void toggle_onoff();
    void toggle_startstop();
    void PollUSB();

private:
    // Device status
    hid_device *device;
    bool isConnected;
    bool onOffStatus;
    bool startStopStatus;
    bool toggleStartStop;
    bool toggleOnOff;

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

    void start_sampling();
    void CloseDevice();

    void save_data(unsigned char* buf);
    char * getDtTm (char *buff);
};

#endif // HID_PNP_H
