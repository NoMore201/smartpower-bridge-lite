# README #

PC bridge software for the Odroid Smart Power meter.
This repo provides the CLI version of the bridge that doesn't require QT libraries.

### Compilation & Execution ###

    ./build.sh

Before executing smartpower, you can get a list of connected devices available using the `--enumerate` option.

**Warning!** In order to send commands through the *libusb* you have to add in /etc/udev/rules.d/99-hiid.rules the following lines:

```
#!shell

#HIDAPI/libusb
SUBSYSTEM=="usb", ATTRS{idVendor}=="04d8", ATTRS{idProduct}=="003f", MODE="0666"
```

### Scripting integration ###

You can easily integrate the executable in your script:

- identify the line at which you launch the benchmark or application you want to measure
- insert the *./SmartPower* command and pass to it the name of the *.csv* file (by default it is "log-<device_path>.csv")
- to stop the measurement kill the SmartPower process through a *SIGUSR1* signal: 

```
#!shell

kill -SIGUSR1 $(pgrep smartpower)
```
