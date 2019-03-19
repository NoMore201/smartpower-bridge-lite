# README #

PC bridge software for the Odroid Smart Power meter.
This software exploits the original graphical interface to provide a tool to automatize measurements and simplify scripting integration.
The software export all logged energy and datetime data to a .csv file.

The '*no_qt*' branch provides a version of the bridge that not depend on the QTCore and QTGui libraries.

### Compilation & Execution ###

* execute '*qmake*' command in the 'smartpower' folder
* execute '*make*' command in the 'smartpower' folder
* go in the '*linux*' folder
* execute the *./SmartPower* executable to start measurements
* you can set the name of the *.csv* file by passing it as an argument to the executable (by default it is "log.csv")

Make sure to have already connected the power meter to the usb port.

** Warning!** In order to send commands through the *libusb* you have to add in /etc/udev/rules.d/99-hiid.rules the following lines:

```
#!shell

#HIDAPI/libusb
SUBSYSTEM=="usb", ATTRS{idVendor}=="04d8", ATTRS{idProduct}=="003f", MODE="0666"
```


### Scripting integration ###

You can easily integrate the executable in your script:

* identify the line at which you launch the benchmark or application you want to measure
* insert the *./SmartPower* command and pass to it the name of the *.csv* file (by default it is "log.csv")
* to stop the measurement kill the SmartPower process through a *SIGUSR1* signal: 

```
#!shell

kill -SIGUSR1 "exec_pid"
```


### Actually in development, more features soon available ###

### Known bugs (will be soon fixed) ###

* If the power meter is not connected via USB the program fails
* Sometimes some data could be incompleted because of communication issues between the power meter and the HID library.