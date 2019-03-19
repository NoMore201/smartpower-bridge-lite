#include "hid_pnp.h"

#include <iostream>
#include <string>
#include <sstream>

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

#define DTTMFMT "%Y-%m-%d %H:%M:%S "
#define DTTMSZ 21

HID_PnP::HID_PnP()
        : device(NULL)
        , isConnected(false)
        , toggleStartStop(0)
        , toggleOnOff(0)
        , current_timeout(STANDBY_POLLING_TIMEOUT)
        , file_name("log.csv")
{
    memset((void*)&buf[2], 0x00, sizeof(buf) - 2);
}


HID_PnP::~HID_PnP() {
}


void HID_PnP::start_sampling() {
   // Create Log file
   std::cout << "File name:" << file_name << std::endl;
   log_file.open(file_name.c_str(), std::ios::out | std::ios::app);

   // Start sampling
   toggleStartStop = true;
   std::cout << "Start sampling" << std::endl;
   time(&start_time);
   while(true){
      PollUSB();
      usleep(current_timeout);
   }
}

void HID_PnP::set_filename(char * const name) {
    file_name = std::string(name);
}

void HID_PnP::stop_sampling() {
   time(&stop_time);
   duration = difftime(stop_time, start_time);
   save_data(buf2);
   // Stop sampling
   std::cout << "Stopping sampling" << std::endl;
   toggleStartStop = true;
   PollUSB();
   CloseDevice();
   log_file.close();
   exit(0);
}

// Done every POLLING_TIMEOUT seconds
void HID_PnP::PollUSB()
{
    buf[0] = 0x00;

    if (isConnected == false) { //Connecting device
        device = hid_open(0x04d8, 0x003f, NULL);

        if (device) { //if device is plugged
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

void HID_PnP::save_data(unsigned char* buf){
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

void HID_PnP::toggle_startstop() {
    start_sampling();
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

