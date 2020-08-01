# PlayStation 3 Controller
Although the procedure in this chapter is written for [the Wilson project](https://www.iambobot.com/en/articles/article_wilson_010_ps3.php) it is actually a generic way for integrating the PlayStation 3 Controller with a Raspberry.

After this setup you will be able to steer the Wilson’s vehicle using any of the 3 joysticks on the PS3 controller: left, right, or gyroscopic (by tilting the controller). Or you can modify the code here to do something different with the joystick information in your own project.

You need to do the following:
1. Tell the controller that the Raspberry is the new master
2. Connect the PS3 Controller to the Raspberry via Bluetooth
3. Install the PS3 Controller driver (sixad)
4. Download and compile the joystick C++ code

A more detailed explanation of the procedure can be found at [the Wilson project](https://www.iambobot.com/en/articles/article_wilson_010_ps3.php) web site.

### 1. Set the Raspberry as the master
Download  `sixad`
```console
$ wget http://www.pabr.org/sixlinux/sixpair.c
```
Install the USB lib
```console
$ sudo apt-get install libusb-dev
```
Compile `sixad`
```console
$ gcc -o sixpair sixpair.c -lusb
```
With sixpair now compiled, plug the PS3 Controller to the Raspberry using the USB mini cable and execute sixpair
```console
$ sudo /home/pi/bin/sixpair
Current Bluetooth master: b8:27:eb:ce:2b:38
Setting master bd_addr to b8:27:eb:ce:2b:38
```

### 2. Connect via Bluetooth
Run bluetoothctl
```console
$ sudo bluetoothctl
Agent registered
[bluetooth]#
```
Enable and configure the the agent
```ApacheConf
[bluetooth]# agent on
[bluetooth]# default-agent
[bluetooth]# power on
[bluetooth]# discoverable on
[bluetooth]# pairable on
```
Connect the controller to the system using a USB cable and press the PlayStation button
```ApacheConf
[CHG] Device 00:06:F7:41:75:2E UUIDs: 00001124-0000-1000-8000-00805f9b34fb
[CHG] Device 00:06:F7:41:75:2E UUIDs: 00001200-0000-1000-8000-00805f9b34fb
```
Allow the service authorization request
```ApacheConf
[agent] Authorize service service_uuid (yes/no): yes
```
Discover the controller's MAC address
```ApacheConf
[bluetooth]# devices
Device 00:06:F7:41:75:2E Sony PLAYSTATION(R)3 Controller
```
Trust the controller
```ApacheConf
[bluetooth]# trust 00:06:F7:41:75:2E
Changing 00:06:F7:41:75:2E trust succeeded. push the central PS button.
```
Quit bluetoothctl and disconnect the USB cable. The PS3 Controller is now paired.

### 3. Install the driver
QtSixA is the Sixaxis Joystick Manager used to connect a PS3 Controller to a Linux machine. QtSixA is the GUI (interface), while sixad is the backend C++ code. Out of the QtSixA package we take just the C++ interface.
Follow this intallation procedure:

Download QtSixA and decompress the tar file
```console
$ wget http://sourceforge.net/projects/qtsixa/files/QtSixA%201.5.1/QtSixA-1.5.1-src.tar.gz
$ tar xfvz QtSixA-1.5.1-src.tar.gz
```
The source code requires a patch to compile correctly. Download and compile the patch
```console
$ wget https://bugs.launchpad.net/qtsixa/+bug/1036744/+attachment/3260906/+files/compilation_sid.patch
$ patch ~/QtSixA-1.5.1/sixad/shared.h < compilation_sid.patch
```
Build and install sixad
```console
$ cd QtSixA-1.5.1/sixad
$ make
$ sudo mkdir -p /var/lib/sixad/profiles
$ sudo checkinstall
```
Now sixad is ready. Run sixad as command line ...
```console
$ sudo sixad --start
sixad-bin[507]: started
sixad-bin[507]: sixad started, press the PS button now
```
... and press the PS button. The controller rumbles while these messages appear
```ApacheConf
sixad-sixaxis[511]: started
sixad-sixaxis[511]: Connected 'PLAYSTATION(R)3 Controller (00:06:F7:41:75:2E)' [Battery 02]
```

### 4. C++ code
The Wilson project provides the C++ code to capture events from js0, translate those events into (x, y) coordinates, and communicate through linux sockets with the vehicles' process that generates the PWM signals to power the motors. This code is also able to trigger rumble effects into the controller.

To install this SW proceed as follows:

Download the tarball with the source code, decompress and compile
```console
$ wget http://www.iambobot.com/downloads/wilson_ps3.tar.gz -O ~/downloads/wilson_ps3.tar.gz
$ tar -C / -xvf ~/downloads/wilson_ps3.tar.gz
$ winstall
```
To run the sw:

Start sixad
```console
$ sudo sixad –start
```
Switch on the Controller by pressing the PS button

Once connected execute the SW selecting the axis: 0 for left joystick, 1 for (default) right joystick, or 2 for gyroscope.
```console
$ ps3 -a 1
```
PS3 captures the events from the driver to generates (x,y) that are sent over a linux sockect to the wilson process (or to a processs of your project with a sockect listening to port 4096).
Press the triangle button to "release the breaks" (note the front light turns on) and play with the joystick to move the vehicle.




