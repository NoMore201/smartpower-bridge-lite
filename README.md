# README

PC bridge software for the Odroid Smart Power meter.
This repo provides the CLI version of the bridge that doesn't require QT libraries.

### Building

    ./build.sh

Before executing smartpower, you can get a list of connected devices available using the `--enumerate` option.

**Warning!** In order to send commands through the *libusb* you may have to add the following lines in `/etc/udev/rules.d/99-hid.rules`:

    #HIDAPI/libusb
    SUBSYSTEM=="usb", ATTRS{idVendor}=="04d8", ATTRS{idProduct}=="003f", MODE="0666"

### Execution

    ./smartpower

By launching the executable, it will start the measurement process on all connected smart power devices.

It is possible to control the measurement process through signals:

- `SIGUSR1` to start/stop the measurement
- `SIGURS2` to toggle on/off smart power devices
- `SIGTERM`/`SIGINT` to stop the measurement and close the program

By default it will log values in separate files `log-{usb-device-path}.csv`
