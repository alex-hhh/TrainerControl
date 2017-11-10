# Read data and control a bike trainer

This is a server application allowing to read data from ANT+ devices and
sending them over to a TCP connection.  It can currently read heart rate from
an ANT+ HRM and read power, speed and cadence from an ANT+ FE-C trainer (most
recent trainers support this).  It can also control the resistance of the
trainer by setting the slope.

## Building the application

The application is built on a Windows platform using Visual Studio 2017 (the
Community Edition will work fine).  It could be easily ported to Linux as
well, but this hasn't been done yet.

Download the libusb windows DLL's from the [libusb](https://libusb.info) web
site and unzip the archive somewhere.

Set the `LIBUSB_LIB_DIR` and `LIBUSB_INCLUDE_DIR` environment variables to
point to the include and lib directories for libusb.  For example, if the
archive was unzipped at `c:\libusb-1.0.21`, we have:

* `LIBUSB_INCLUDE_DIR` pointing to `c:\libusb-1.0.21\include`

* `LIBUSB_LIB_DIR` pointing to `c:\libusb-1.0.21\MS32\dll` (point it to the 64
  bit if building a 64 bit application)
  
Open the `vs2017/TrainerControl.sln` solution and build it (if you opened it
before setting up the environment variables, you will need to re-open it).

The resulting executable will be in the `Debug` or `Release` folder.  You will
also need to copy the actual libusb dll file, `libusb-1.0.dll` in that folder.

## Running the application

To run the application, open a command window and type:

    ./TrainerControl.exe
    
The application will try to find the ANT+ USB stick and connect to the heart
rate monitor and bike trainer.  It will also accept TCP connections on port
7500.

